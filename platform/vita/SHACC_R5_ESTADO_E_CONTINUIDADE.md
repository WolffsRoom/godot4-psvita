# Godot4Vita v0.3 — estado técnico e continuidade do compilador de shaders GXM

**Data de consolidação:** 03/11/2026  
**Status:** integração do módulo Sony e diagnóstico nativo funcionais; tradutor Godot ShaderLanguage → Cg e renderização final ainda não validados no Vita.  
**Origem:** investigações em hardware PS Vita de Split Memories e ShaderTest da Godot4Vita.  
**Escopo:** ENGINE Godot4Vita v0.3, não um patch específico do jogo.

## 1. Objetivo e restrições

Implementar suporte NATIVO aos recursos ShaderMaterial/canvas_item do Godot 4.x no backend Sony GXM da Godot4Vita, utilizando vitaShaRK + libshacccg.suprx do próprio usuário para compilar Cg em GXP e vincular o fragment program ao sceGxmShaderPatcher. Preservar os shaders originais dos jogos. Após o pipeline simples funcionar, expandir para screen textures, mip/LOD, fog, CRT, vidro, lights e occluders.

Regras: não distribuir libshacccg.suprx em VPK, não substituir o shader original por imagens/efeitos falsos, não anunciar paridade com Godot PC prematuramente e não modificar os builds estáveis V1/V2/V3 do Split Memories.

## 2. Ambiente e caminhos

- Windows: `C:\Users\wolff\Documents\SDKVita`.
- Engine: `GodotPSV\godot-master`; plataforma: `GodotPSV\godot-master\platform\vita`.
- Editor local: `GodotPSV\editor-vita-integrado\Godot4Vita.exe`.
- Template padrão instalado no editor: `GodotPSV\editor-vita-integrado\vita\templates\vita_release.zip`. ATENÇÃO: não sobrescrever sem nova validação/backup.
- WSL VitaSDK: `/usr/local/vitasdk`; VitaGL local usado nos builds: `GodotPSV\_build\ubo-20261010-02\vitagl`.
- ShaderTest R5: `GodotPSV\ShaderTest-v0.3\Diagnostic-R5`.
- Build/template R5: `GodotPSV\_build\shacc-v03\fix-20261102\patch-r5`.
- Runtime GODOT GXM R4 (usado nos cinco R5): `GodotPSV\_build\shacc-v03\fix-20261102\patch-r4\libgodot.so`.
- Projeto de referência Split Memories SEM ALTERAR shaders: `Split Memories Vita Port\Dev\GXM-Shader-Implementation\project-unmodified-effects`.
- Split Memories R4: `Split Memories Vita Port\Dev\GXM-Shader-Implementation\R4`.
- Logs Vita anteriores R4: `GodotPSV\ShaderTest-v0.3\Diagnostic-R4\logs\CG_PROBE`.
- Logs dos cinco R5: AINDA NÃO FORNECIDOS. Ao recebê-los, sugerir `GodotPSV\ShaderTest-v0.3\Diagnostic-R5\Logs\0` até `Logs\4`, ou localizar recursivamente os arquivos reais.

## 3. Implementação efetiva na engine

| Arquivo | Função |
|---|---|
| `platform/vita/loader/shacc_runtime.c` / `.h` | Serviço nativo compartilhado: valida SELF Sony, tamanho, CRC32, inicializa vitaShaRK, compila Cg, registra falhas, libera programas, diagnósticos module_only/early/cg_probe_N |
| `platform/vita/loader/gxm_bridge.c` | Registra GXP compilado no shader patcher, desenha fragment program GXM, faz cache de falhas por hash |
| `platform/vita/gxm_shader_translator_vita.inc` | Tradução EXPERIMENTAL de subconjunto Godot `canvas_item` → Cg; cobertura incompleta |
| `platform/vita/gxm_canvas_vita.inc` | Render 2D GXM; prepara materiais, agenda compilação fora de cena ativa, uniformes, fallback transparente para overlays sem shader GXP |
| `platform/vita/rasterizer_scene_gxm_vita.inc` | Armazenamento material/shader source, parâmetros, RID, screen texture |
| `platform/vita/gxm_bridge.h` e `loader/default_dynlib.c` | API/imports compartilhados entre engine SO e loader |
| `platform/vita/editor/addons/psvita/platform.gd` | Export: Rendering / Shader Compiler, modos disabled/safe/experimental, `shacc.cfg` por VPK |
| `platform/vita/tools/make_editor_template.py` | Manifesto com `runtime_shacc: v1` e validações |
| `platform/vita/SHADER_COMPILER.md` | Manual da integração, limitações e histórico de diagnósticos |

O módulo do Vita é procurado prioritariamente em `ur0:/data/libshacccg.suprx` e alternativamente em `ur0:/data/external/libshacccg.suprx`. Em hardware físico, a cópia validada tinha 3.407.920 bytes, CRC32 `C9D1FD85`. **Presença/CRC não garantem que todas as fontes Cg compilarão.**

No modo `disabled` não deve carregar o compilador; `safe` aplica limites de fonte e restringe loops/recursos não suportados; `experimental` é para pesquisa. O carregamento antecipado também foi isolado como diagnóstico.

## 4. Linha do tempo — evidências do PS Vita

**LAB inicial / R2:**
- Menu do Split Memories branco, áudio e navegação funcionando; Play causou crash.
- Dump no thread `SceShaccCg EntryCall`, exceção `0x30003` (prefetch abort), PC `0xE0001876`, IFAR `0`.
- No código inicial, a compilação nativa era solicitada durante desenho GXM; R2 mudou para fila antes de `sceGxmBeginScene`, adicionou verificação de módulo e logs. Isso NÃO comprovou estabilidade.

**Godot4Vita v0.3 e R3:**
- VPK `MODULE_ONLY`: sem crash, log `[shacc-boot] END rc=1` (carregamento do módulo Sony).
- VPK `EARLY_INIT`: sem crash na inicialização, log `[shacc-boot] END rc=2` (vitaShaRK inicializado antes da engine).
- ShaderTest de fragmentos UV, uniform e TIME: o código traduzido foi enviado ao Sony Cg, mas retornou `fatal internal error` e `program=0x0`.
- Split Memories R3: menu voltou a aparecer sem o ColorRect de screen texture branco; efeito glass_broken ainda ausente. Fase mostrou metade verde sólida.

**R4:**
- Identificação visual e em `level_1.tscn`: `Side_2/Control/ColorRect` ocupa 224×272, `z_index=1`, tem `ShaderMaterial` com `res://resources/shaders/fog.gdshader`. Como o programa nativo não estava compilado, o renderer desenhava um fallback opaco, encobrindo a metade direita; **isso não demonstra que PointLight2D era a causa do bloco verde**.
- Fallback genérico do GXM agora suprime o desenho de retângulos SEM textura com ShaderMaterial sem GXP, mantém sprites texturizados, não modifica o shader original. Correção visual R4 ainda aguarda confirmação em Vita.
- Introduzido `cg_probe` com cinco fontes Cg mínimas, compiladas diretamente na inicialização ANTES do Godot. Hardware: `godot_loader.log` registrou `[shacc-boot] END rc=3`. Segundo a função de diagnóstico, `rc=3` exige **pelo menos uma fonte compilada com retorno de GXP não nulo e tamanho positivo**. Não identifica QUANTAS nem QUAL.
- Posteriormente UV gradient, uniforms e TIME produzidos pelo tradutor continuaram retornando `fatal internal error`.
- Os cinco resultados individuais do R4 não foram persistidos: foram escritos em stderr antes de seu redirecionamento pelo Godot.

**R5 — versão atual de diagnóstico:**
- Ajustado `shacc_runtime.c` para registrar `BEGIN/END`, callback de compilação, fonte Cg, tamanho e resultado em `godot_loader.log` via `debugPrintf`, já disponível desde o boot.
- `shacc_diag.cfg` seleciona `cg_probe_0` até `cg_probe_4`. **Apenas UM shader Cg por VPK**, isolando estados do compilador.
- Quando Cg retorna GXP, o serviço grava `cg_probe_N.gxp` no diretório de logs do aplicativo.
- Loader ARM recompilado. Runtime `libgodot.so` R4 **reutilizado sem alterações**, de propósito: o diagnóstico R5 altera somente o loader.
- Template novo: `patch-r5\vita_release_v03_R5.zip`. VPKs ZIP/PCK/hash testados; hardware dos cinco R5 **PENDENTE**.
- Nenhum `libshacccg.suprx` está embutido.

## 5. Matriz dos cinco VPKs R5

| Prova | VPK | Title ID | Logs esperados |
|---|---|---|---|
| 0 | Godot4Vita-v0.3-CG_PROBE_0-R5.vpk | GVSD00040 | ux0:data/shaccprobe0r5/logs/ |
| 1 | Godot4Vita-v0.3-CG_PROBE_1-R5.vpk | GVSD00041 | ux0:data/shaccprobe1r5/logs/ |
| 2 | Godot4Vita-v0.3-CG_PROBE_2-R5.vpk | GVSD00042 | ux0:data/shaccprobe2r5/logs/ |
| 3 | Godot4Vita-v0.3-CG_PROBE_3-R5.vpk | GVSD00043 | ux0:data/shaccprobe3r5/logs/ |
| 4 | Godot4Vita-v0.3-CG_PROBE_4-R5.vpk | GVSD00044 | ux0:data/shaccprobe4r5/logs/ |

Fontes de Cg (verificar a versão exata em `platform/vita/loader/shacc_runtime.c`):

0. `float4 main(uniform float4 uClearColor) : COLOR { return uClearColor; }` — uniform no argumento da função, caso mais próximo da referência Cg.
1. `float4 main() : COLOR { return float4(1.0, 0.0, 0.0, 1.0); }` — cor constante.
2. `float4 main(float2 uv : TEXCOORD0) : COLOR { return float4(uv.x, uv.y, 0.2, 1.0); }` — TEXCOORD0.
3. `uniform float4 gxm_tint; float4 main(float2 uv : TEXCOORD0) : COLOR { float4 COLOR = float4(uv.x, uv.y, 0.2, 1.0); return COLOR * gxm_tint; }` — uniform global.
4. `uniform sampler2D TEXTURE : TEXUNIT0; uniform float4 gxm_tint; float4 main(float2 uv : TEXCOORD0) : COLOR { return tex2D(TEXTURE, uv) * gxm_tint; }` — sampler e texture lookup.

Cada teste: abrir VPK; **não é preciso apertar X**; copiar `godot_loader.log`, `godot_stderr.log`, eventual `cg_probe_N.gxp` e `psp2dmp`.

Logs esperados:
`[vita-shacc] cg_probe version=R5 selected=N total=1 mode=safe`
`[vita-shacc] cg_probe #N BEGIN bytes=... source=...`
`[vita-shacc] compiler level=...` (se erro)
`[vita-shacc] cg_probe #N END gxp=0|1 gxp_bytes=...`
`[vita-shacc] cg_probe RESULT passes=0|1 total=1 selected=N`
`[shacc-boot] END rc=3` (sucesso nesse Cg) ou `rc=-5` (falha).

O arquivo `.gxp` é saída local do compilador; NÃO prova por si só renderização no GXM.

## 6. Procedimento de continuidade após chegada dos cinco logs

1. Localizar todos os resultados R5 **reais** nas pastas montadas. Ler `godot_loader.log` + `godot_stderr.log` integralmente; não inferir ausência de log nem sucesso por jogo abrir. Validar `BUILD_ID` e `Title ID` por VPK; evitar misturar R4 e R5.
2. Produzir uma tabela 0–4: log encontrado? init_ok? `cg_probe END gxp`, bytes, erro exato, `gxp` existente/tamanho, crash dump? Distinguir log incompleto e retorno de compilação inválido.
3. Se algum `gxp=1`, analisar seu Cg original e `.gxp`; identificar primeiro menor conjunto que compilou. Com testes progressivos, determinar sintaxe Cg aceita, sem adivinhar sobre fonte não testada.
4. Corrigir o tradutor `gxm_shader_translator_vita.inc` e/ou opções de `shark_compile_shader_extended` somente com evidência, mantendo backups e guardas de compilação. **Não alterar arquivos `.gdshader` dos jogos.**
5. Criar smoke GXM mostrando output do fragment program nativo, checar `sceGxmShaderPatcherRegisterProgram`, criação de fragment program compatível com vertex program, binding de uniformes/texturas, blending, formato de target. Um `GXP válido` não garante GPU correta.
6. Validar UV, depois uniform, TIME, sampler2D. Só depois avançar a `hint_screen_texture`, capturas/mips, `fog.gdshader`, `glass_broken` e iluminação/sombras `PointLight2D` / `LightOccluder2D`.
7. Compilar loader VitaSDK e runtime ARM via SCons quando necessário; exportar novos VPKs lado a lado com TitleID e pasta de logs únicos, testar no host e documentar limites.
8. Salvar relatório com evidências, alterações, resultados e próximos bloqueios. Nunca dizer “funcional” apenas porque `rc=3`; exigir frame visual confirmado no PS Vita físico.

### Matriz de decisão

- **R5 todos falham:** examinar divergência de opções/shark_init, estado de dependências, memória/allocator e logs antes do init; R4 rc=3 aponta que pelo menos uma compilação anterior funcionou — investigar por que difere.
- **Apenas 0/1 passam:** isolar se `TEXCOORD0`, uniform global e/ou `sampler2D/TEXUNIT0` são incompatíveis; trabalhar o tradutor a partir do caso aceito.
- **0–4 passam:** focar nas transformações GLSL→Cg produzidas pelo tradutor; testar a fonte efetiva gerada e o backend do patcher.
- **GXP compila mas tela branca/sem efeito:** inspecionar GXM vertex/fragment linkage, emissão de vertex, uniform/fog/sampler binding e a ordem de renderização, não só a fonte Cg.
- **Crash nativo:** preservar dump/último log, priorizar correção da engine e teste mínimo, nunca esconder o erro com alterações do jogo.

## 7. Build e verificações

Script de pacote R5 existente:
`GodotPSV\_build\shacc-v03\fix-20261102\patch-r5\package-r5.sh`

Script de geração dos cinco VPKs:
`GodotPSV\_build\shacc-v03\fix-20261102\patch-r5\package_single_probes.py`

Build de engine (se a engine `.cpp` mudar), no WSL:
`cd /mnt/c/Users/wolff/Documents/SDKVita/GodotPSV/godot-master`
`export VITASDK=/usr/local/vitasdk`
`python3 -m SCons platform=vita arch=arm32 target=template_release optimize=size disable_path_overrides=no vita_shared=yes vita_gxm3d=yes vita_gl_root=/mnt/c/Users/wolff/Documents/SDKVita/GodotPSV/_build/ubo-20261010-02/vitagl -j6`

Validar `platform/vita/tools/verify_runtime.py`, ZIP `testzip()`, manifest `runtime_shacc=v1`, integridade de `eboot.bin`, `libgodot.so` e PCK, além de possíveis alterações acidentais nas versões estáveis. Testes do host **não** substituem hardware.

## 8. Pendências importantes

- Confirmar em hardware quais probes R5 0–4 passaram.
- Corrigir a tradução Cg nativa para shaders Godot simples.
- Confirmar GXP registrado e desenhado no GXM.
- Suporte completo a variações vertex/varying, funções ShaderLanguage, sampler/textureLod, render targets screen/mipmap e blending.
- Godot 2D `PointLight2D`/`LightOccluder2D`, fog procedural, CRT e vidro quebrado com fidelidade.
- Verificar Split Memories R4 visualmente. A correção do retângulo verde evita sobreposição indevida, **não recria o fog** e ainda não foi validada no console.
- Só após testes reais considerar integração no template/release v0.3 padrão; o template atual do editor não deve ser alterado sem aprovação.

## 9. Artefatos e documentação complementares

- `GodotPSV\godot-master\platform\vita\SHADER_COMPILER.md`
- `GodotPSV\_build\shacc-v03\fix-20261102\patch-r4\DIAGNOSTICO-R4.md`
- `GodotPSV\_build\shacc-v03\fix-20261102\patch-r5\RESULTADO-R4-E-PLANO-R5.md`
- `GodotPSV\ShaderTest-v0.3\Diagnostic-R5\MANIFEST-R5.json`
- `GodotPSV\ShaderTest-v0.3\Diagnostic-R5\LEIA-ME-R5.txt`
- `Split Memories Vita Port\Dev\GXM-Shader-Implementation\R4\LEIA-ME-R4.txt`

Este relatório distingue observações no hardware de inferências e de implementações não concluídas. A próxima IA deve partir dos logs R5, não da hipótese de que todos os shaders funcionaram.

## 10. Atualização R6 — 03/11/2026

Logs R5 recebidos posteriormente confirmaram em hardware os probes 0, 1 e 2: uniform como parâmetro de `main` (232 bytes), cor constante (220 bytes) e `TEXCOORD0` (244 bytes). Os logs 3 e 4 não estavam presentes; uniform global e sampler continuam indeterminados.

O tradutor foi ajustado para seguir a forma comprovada: uniforms passam a parâmetros de `main`, built-ins são emitidos somente quando usados e `TEXTURE` deixa de ser injetada em shaders puramente procedurais. A bridge ganhou telemetria explícita para registro no patcher, criação do fragment program e primeira submissão real de draw.

Runtime e loader R6 compilaram e passaram no verificador de ELF/relocações/imports. Três VPKs isolados foram gerados em `ShaderTest-v0.3/Diagnostic-R6`, com Title IDs `GVSD00050` a `GVSD00052`. O resultado visual em hardware ainda é obrigatório antes de classificar UV, uniforms ou TIME como funcionais. Relatório completo: `ShaderTest-v0.3/Diagnostic-R6/RESULTADO_R5_E_TESTE_R6.md`.
