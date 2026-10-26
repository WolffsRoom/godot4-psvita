"""Exercise the packaged editor's VPK/data export contract without opening its UI."""
from pathlib import Path
import hashlib
from datetime import datetime
import json
import os
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[4]
EDITOR = ROOT / 'editor-vita-integrado/Godot4Vita.exe'
WORK = ROOT / '_build/split-export-tests' / datetime.now().strftime('%Y%m%d-%H%M%S')
PROJECT = WORK / 'project'
PROJECT.mkdir(parents=True, exist_ok=True)
for name in ('project.godot', 'main.gd', 'main.tscn', 'vita_icon.png'):
    shutil.copy2(ROOT / 'editor-vita-integrado/Exemplo' / name, PROJECT / name)
base = (ROOT / 'editor-vita-integrado/Exemplo/export_presets.cfg').read_text()
base = '\n'.join(line for line in base.splitlines() if not line.startswith('data/'))
env = dict(os.environ, APPDATA=str(WORK/'appdata'), LOCALAPPDATA=str(WORK/'localappdata'))
results = {}

def export(label, separate=None, folder='meujogovita', pack=False, destination=None, template=None, success=True):
    preset = base + '\ndata/folder_name=' + json.dumps(folder) + '\n'
    if separate is not None:
        preset += 'data/separate_folder=' + str(separate).lower() + '\n'
    if template:
        preset = preset.replace('custom_template/release=""', 'custom_template/release=' + json.dumps(template.as_posix()))
    (PROJECT/'export_presets.cfg').write_text(preset)
    destination = destination or WORK/label/('MeuJogo.pck' if pack else 'MeuJogo.vpk')
    destination.parent.mkdir(parents=True, exist_ok=True)
    log = WORK/(label+'.log')
    run = subprocess.run([str(EDITOR), '--headless', '--path', str(PROJECT), '--export-pack' if pack else '--export-release', 'PS Vita', str(destination), '--log-file', str(log)], env=env, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
    text = log.read_text(encoding='utf-8')
    assert 'SCRIPT ERROR' not in text, text
    if success:
        assert run.returncode == 0 and 'VPK: ' in text, text
    else:
        assert 'PS Vita' in text and ('ERROR' in text or 'Error' in text), text
    results[label] = 'passed'
    return destination.with_suffix('.vpk'), text

split, _ = export('default')
data = split.parent/'meujogovita'
with zipfile.ZipFile(split) as z:
    assert z.testzip() is None
    assert z.read('game_data_folder.cfg') == b'meujogovita'
    assert not any(n.startswith('game_data/') for n in z.namelist())
assert (data/'project.binary').is_file()
assert list(data.rglob('*.ctex'))
assert (data/'.vita-export-owner').read_text() == 'GDOT40001'
full, _ = export('full', separate=False)
with zipfile.ZipFile(full) as z:
    assert z.testzip() is None
    assert 'game_data_folder.cfg' not in z.namelist()
    assert z.read('game_data/project.binary') == (data/'project.binary').read_bytes()
    for file in data.rglob('*'):
        if file.is_file() and file.name != '.vita-export-owner':
            assert file.read_bytes() == z.read('game_data/'+file.relative_to(data).as_posix())
assert not (full.parent/'meujogovita').exists()
(data/'previous-export-only.txt').write_text('preserve me')
export('repeat', destination=split)
backups = list(split.parent.glob('meujogovita.previous-*'))
assert len(backups) == 1 and (backups[0]/'previous-export-only.txt').read_text() == 'preserve me'
assert not (data/'previous-export-only.txt').exists()
packed, _ = export('pack', pack=True)
assert packed.is_file() and not packed.with_suffix('.pck').exists()
zipped, _ = export('zip', pack=True, destination=WORK/'zip/MeuJogo.zip')
assert zipped.is_file() and not zipped.with_suffix('.zip').exists()
export('invalid', folder='../escape', success=False)
assert not (WORK/'invalid/MeuJogo.vpk').exists()
collision = WORK/'collision/meujogovita'
collision.mkdir(parents=True, exist_ok=True)
(collision/'user.txt').write_text('keep')
export('collision', success=False)
assert (collision/'user.txt').read_text() == 'keep'
# A failed VPK publication must roll back a replaced data folder.
rollback = WORK/'rollback'
shutil.copytree(data, rollback/'meujogovita', dirs_exist_ok=True)
(rollback/'meujogovita/keep.txt').write_text('previous')
(rollback/'MeuJogo.vpk').mkdir(exist_ok=True)
export('rollback', success=False)
assert (rollback/'meujogovita/keep.txt').read_text() == 'previous'
# A template without the capability must refuse split export, keeping full mode compatible.
old = WORK/'legacy-template.zip'
with zipfile.ZipFile(ROOT/'editor-vita-integrado/vita/templates/vita_release.zip') as src, zipfile.ZipFile(old,'w',zipfile.ZIP_DEFLATED) as dst:
    for name in src.namelist():
        raw = src.read(name)
        if name == 'vita-template.json':
            manifest = json.loads(raw); manifest.pop('external_game_data', None)
            raw = json.dumps(manifest).encode()
        dst.writestr(name,raw)
export('legacy-split', template=old, success=False)
export('legacy-full', template=old, separate=False)
# Run the exported loose resources on the host to check completeness.
log = WORK/'host-startup.log'
run = subprocess.run([str(EDITOR),'--headless','--path',str(data),'--quit-after','120','--log-file',str(log)],env=env,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
text = log.read_text(encoding='utf-8')
assert run.returncode == 0 and 'SCRIPT ERROR' not in text and 'READY' in text, text
results['loose_data_host_startup'] = 'passed'
results['physical_vita'] = 'pending'
results['split_vpk_bytes'] = split.stat().st_size
results['full_vpk_bytes'] = full.stat().st_size
(WORK/'results.json').write_text(json.dumps(results,indent=2)+'\n')
(ROOT/'_build/split-export-results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
