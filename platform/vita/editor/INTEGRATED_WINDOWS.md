# Integrated Windows editor

`modules/vita_editor` registers a built-in EditorPlugin and a scripted
EditorExportPlatformExtension for every project. The export scripts, template,
defaults and preview assets are located in `vita/` beside the executable.
Project-local addon installation is not required. Existing updated addons detect
the native plugin and avoid registering a second platform.

The native platform defaults new presets to text scripts. The inspector offers
Vita-specific project overrides and portable defaults resolved beside the exe.
It does not silently change existing presets or project settings on startup.

## Build

Run `platform/vita/tools/build_windows_editor.sh` using WSL with MinGW x86-64
POSIX and SCons. This builds in `/tmp/godot-vita-windows-editor`, leaving the
Vita checkout's generated headers and objects untouched. The build is Windows
x86-64, editor, Compatibility/OpenGL, dev7, custom build name `vita_editor`.

Run `platform/vita/tools/package_windows_editor.py` with Python to assemble
support files in `GodotPSV/editor-vita-integrado`. The tested canonical template
is taken from `editor-vita/ProjetoVita/addons/psvita/templates/vita_release.zip`.
Keep `vita/` next to `Godot4Vita.exe`; the distribution is a portable directory,
not a single-file installer. `_sc_` enables self-contained editor settings.

## Validation

Import and export `editor-vita-integrado/Exemplo` with the compiled exe. This
project intentionally has no addon. Confirm the resulting VPK's CRCs, manifest
hashes, indexed PNG8 assets, a5 XML, imported resources and canonical boot DAT.
Also test relocation and a newly created native preset (script mode Text).
Windows headless export validation does not certify physical Vita gameplay.

## VPK e dados separados

A exportação sempre gera um `.vpk`, inclusive pela opção Exportar PCK/ZIP (a extensão de saída é ajustada para `.vpk`).
**Data > Separate Folder** vem ativado. **Data > Folder Name** define uma pasta exclusiva do jogo, com 1–64 letras, números, hífen ou sublinhado. Não use caminhos completos nem nomes reservados do Windows.

Exemplo: salve `MeuJogo.vpk` e informe `meujogovita`. Serão gerados lado a lado o VPK e a pasta `meujogovita`. Instale o VPK pelo VitaShell e copie a pasta inteira para `ux0:data/`, resultando em `ux0:data/meujogovita/project.binary` e os demais recursos. Preserve arquivos ocultos e a pasta `.godot` exportada. O nome diferencia maiúsculas e minúsculas; mantenha a grafia exata. O VPK ainda inclui o runtime, a animação e os assets LiveArea.

Desmarque **Separate Folder** para incluir todos os dados dentro do VPK, sem cópia separada no Vita. Um template antigo sem suporte a dados externos só permite esse modo completo.

Ao reexportar, a pasta anterior gerada para o mesmo Title ID é preservada com sufixo `.previous-...`; pode ser removida após conferir a nova exportação. Uma pasta preexistente sem identificação do exportador não é substituída. Pastas `.partial-...` indicam uma tentativa incompleta e não devem ser copiadas para o Vita.

## Trophy panel

The native plugin registers trophy_panel.gd in the editor bottom panel. trophy_tools.gd handles the project configuration, Steam schema mapping, indexed PNG encoding, SFM and TRP v2 generation. The export platform builds the enabled package directly into sce_sys/trophy/<communication_id>/TROPHY.TRP in both export modes. See TROFEUS.md for the UI flow and the pending native unlock bridge.

Validation: editor/test_trophies.gd, editor/test_trophy_native.gd, tools/test_trophy_export.py and tools/tests/test_trophy_trp.py. Network import success requires a real user API key; the automated import tests use a representative Steam schema fixture.

