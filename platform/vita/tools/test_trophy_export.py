"""End-to-end trophy export with the staged native editor; no UI automation."""
from pathlib import Path
import json,os,subprocess,zipfile,hashlib
ROOT=Path(__file__).resolve().parents[4]
work=ROOT/'_build/trophy-tests'; project=work/'project'
editor=ROOT/'_build/trophy-editor/Godot4Vita.exe'
env=dict(os.environ,APPDATA=str(work/'appdata'),LOCALAPPDATA=str(work/'localappdata'))
def run(args,log):
 result=subprocess.run([str(editor),'--headless','--path',str(project),*args,'--log-file',str(work/log)],env=env,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
 text=(work/log).read_text(encoding='utf-8');assert result.returncode==0 and 'SCRIPT ERROR' not in text,text
 return text
text=run(['--editor','--script',str(ROOT/'godot-master/platform/vita/editor/test_trophies.gd'),'--quit-after','100'],'unit-final.log')
assert 'PASS: trophy UI' in text
text=run(['--editor','--script',str(ROOT/'godot-master/platform/vita/editor/test_trophy_native.gd'),'--quit-after','100'],'native-final.log')
assert 'PASS: exactly one' in text
config=project/'vita_trophies/trophies.json'
model=json.loads(config.read_text(encoding="utf-8"))
preset=project/'export_presets.cfg'; original=preset.read_text()
reference=(project/'vita_trophies/vpk_inject/sce_sys/trophy/TEST40001_00/TROPHY.TRP').read_bytes()
for label,separate,enabled in [('split',True,True),('full',False,True),('disabled',False,False)]:
 model['enabled']=enabled;config.write_text(json.dumps(model),encoding="utf-8")
 preset.write_text(original.replace('data/separate_folder=true','data/separate_folder='+str(separate).lower()))
 dest=work/(label+'.vpk')
 run(['--export-release','PS Vita',str(dest)],label+'.log')
 with zipfile.ZipFile(dest) as z:
  assert z.testzip() is None
  trps=[n for n in z.namelist() if n.endswith('TROPHY.TRP')]
  assert trps==(['sce_sys/trophy/TEST40001_00/TROPHY.TRP'] if enabled else [])
  if enabled: assert z.read(trps[0])==reference
  assert not any('vita_trophies/' in n for n in z.namelist())
  assert ('game_data/project.binary' in z.namelist())== (not separate)
model['enabled']=True;config.write_text(json.dumps(model,indent=2),encoding="utf-8");preset.write_text(original)
report={'native_trophy_tab':'passed','steam_schema_import_and_merge':'passed','png8_and_generation':'passed','split_and_full_vpk_injection':'passed','disabled_excludes_trp':'passed','live_steam_api':'not_tested_without_user_key','physical_vita':'pending','native_unlock_bridge':'not_implemented','editor_sha256':hashlib.sha256(editor.read_bytes()).hexdigest()}
(work/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
