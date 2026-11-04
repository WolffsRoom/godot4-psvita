# VitaGL x GXM nativo: backend A/B e benchmark

Revisão: 2026-11-04. Estado: **implementado e testado no host com API
simulada; não compilado para ARM e não executado no Vita.** Nenhum número de
desempenho deste documento foi medido no aparelho.

## O que existe hoje

| Caminho | Uso atual | Situação |
|---|---|---|
| Canvas GXM nativo (`gxm_canvas_vita.inc` → `loader/gxm_bridge.c` → libvita2d/GXM) | Todos os jogos 2D e a UI dos jogos 3D | Em produção |
| Protótipo 3D GXM (`rasterizer_scene_gxm_vita.inc` → mesma ponte) | Overburden, Buckshot Roulette | Malhas residentes na GPU, programas da libvita2d com MVP real |
| Compatibility GLES3 sobre VitaGL (`drivers/gles3` + `gl_manager_vita.cpp`) | Nenhum jogo | Parado desde 2026-10-12: `CanvasShaderGLES3` falha no compilador |

O 2D e o 3D atuais **não usam VitaGL**. Ela continua vinculada ao loader
porque o `libgodot.so` também importa as funções `gl*` do caminho GLES3.

## Ponte VitaGL (`loader/gxm_bridge_vgl.c`)

Implementa o mesmo contrato `vita_gxm_*` da ponte nativa sobre VitaGL,
selecionada com `-DVITA_VITAGL_BRIDGE=ON` no loader. O `libgodot.so` e todo o
código Godot do renderer ficam idênticos; só muda o `eboot.bin`. A comparação
fica justa: mesmo jogo, mesmos comandos, mesmo batching e mesmo culling.

- Dois programas GLSL ES 1.00 mínimos (cor e textura com tint), compilados uma
  vez no init. Não usa os shaders GLES3 do Godot. Os vértices têm o mesmo
  layout da libvita2d (x, y, z + cor ou uv).
- 2D: batching igual ao nativo, recorte por `glScissor`, SubViewport por FBO,
  blend da libvita2d (`SRC_ALPHA/ONE_MINUS_SRC_ALPHA`, aditivo `ONE/ONE`),
  RGBA8 e RGBA4444.
- 3D: malhas residentes como VBO (mesmo teto de 24 MiB da arena nativa),
  triângulos de CPU, profundidade `LEQUAL`, passe transparente sem escrita de
  profundidade, passe de iluminação por vértice com multiplicação 2x e glows
  aditivos.
- Sem MSAA (a libvita2d também não usa).
- `[vitagl-perf]` tem os mesmos campos de `[gxm2d-perf]`. A ponte nativa
  ganhou `submit_us` no fim da linha. Nas duas, `draws`/`submit_us` cobrem só
  os lotes do canvas; o 3D se compara pelo `cpu_submit_us` das linhas
  `[gxm3d]`, que o engine mede igual nos dois casos.

Limites:

- Tilt-shift não existe nesta ponte: o engine desenha o quadro normal.
  Benchmarks devem manter `rendering/vita/tilt_shift=false`.
- Exige `libshacccg.suprx` no console (a ponte nativa não exige).
- Se a compilação dos shaders falhar depois do `vglInit`, não há fallback para
  libvita2d: o GXM já pertence a VitaGL. O log informa a causa.
- A orientação de FBO segue a convenção GL da build sem
  `HAVE_UNFLIPPED_FBOS` (lida em `framebuffers.c`/`misc.c` do commit fixado).
  Se a SubViewport aparecer invertida, use `target_flip=0` no `vitagl.cfg`.
- Atualizar uma malha ou as cores de iluminação reenvia o VBO inteiro
  (`glBufferData`); a ponte nativa reescreve a memória da GPU direto.

## Avaliação: deve haver ganho?

**Hipótese, baseada na leitura do código da libvita2d e da VitaGL do commit
`df63ce8`: não. O esperado é empate na GPU e mais custo de CPU e memória com
VitaGL, nos dois modos.** O benchmark abaixo confirma ou derruba isso.

- **CPU por draw.** A libvita2d escreve os vértices direto na memória mapeada
  e cada draw é programa, textura, uniform buffer e `sceGxmDraw`. Na VitaGL,
  `glDrawArrays` também copia os vértices client-side para o circular pool,
  valida o estado sujo, procura o fragment program pelo estado de blend e
  copia todos os uniforms. A build fixada (`NO_DEBUG=0`, `LOG_ERRORS=2`)
  ainda checa erros GL em cada chamada.
- **GPU.** Mesma SGX543, mesma resolução, shaders triviais equivalentes e a
  mesma quantidade de fragmentos. O recorte também não favorece VitaGL: a
  libvita2d desenha dois retângulos de stencil, e a VitaGL desenha um quad de
  profundidade (`update_scissor_test`) mais o region clip por tile.
- **Memória.** A VitaGL reserva no init quase toda a memória livre de cada
  pool (menos os limites configurados) e cria um circular pool (32 MiB por
  padrão; a ponte usa 8 MiB e deixa 32 MiB de USER_RW e 8 MiB de phycont
  livres). A libvita2d aloca por textura.
- **3D.** Com malhas já residentes nos dois casos, a diferença fica no custo
  por draw e no reenvio das cores de iluminação por quadro. Um ganho real no
  3D viria de shaders GXM dedicados, e isso não depende de VitaGL.
- **Onde VitaGL ajuda.** Funcionalidades: GLSL compilado em tempo de
  execução abriria caminho para shaders de `CanvasItem`/materiais. Não está
  nesta ponte.

## Benchmark (os dois VPKs "Showcase")

Projeto: `tests/vitagl_benchmark` (procedural, sem assets importados, vsync
desligado para medir acima de 60 FPS, budget de triângulos alto). Oito fases,
cada uma com 2 s de aquecimento e 10 s de medição:

| Fase | O que estressa |
|---|---|
| `2d_sprites_same_texture` | 1000 sprites, mesma textura: poucos draws, muitos quads |
| `2d_sprites_mixed_textures` | 1000 sprites alternando 8 texturas: um draw por sprite (custo por draw) |
| `2d_ui_text_clip` | 24 painéis com `clip_contents` e texto rolando: recortes e glifos |
| `2d_subviewport` | SubViewport 480x272 com 300 sprites, ampliada na tela |
| `3d_boxes_textured` | 64 caixas texturizadas sem luz, câmera orbitando |
| `3d_spheres_dense` | 256 esferas (~65 mil triângulos) |
| `3d_vertex_lit` | Luz direcional + 2 omni móveis: iluminação por vértice recalculada todo quadro |
| `3d_transparent` | 64 caixas com alpha: passe transparente |

Gerar (WSL, VitaSDK, VitaGL com o patch):

```sh
platform/vita/tools/build_vitagl_benchmark.sh /caminho/vitagl /caminho/saida [/caminho/vitagl-release]
```

O script compila o runtime com `vita_gxm3d=yes` (ou reutiliza
`VITA_RUNTIME_SO`), monta os dois loaders e confere que os VPKs têm o mesmo
`libgodot.so`. Saída: `benchmark_gxm.vpk` (**Bench GXM**, `GDBG00001`),
`benchmark_vitagl.vpk` (**Bench VitaGL**, `GDBV00001`) e
`benchmark-manifest.json`. O terceiro argumento opcional é uma VitaGL do mesmo
commit e patch compilada com `NO_DEBUG=1` e sem `LOG_ERRORS`, para não
penalizar a VitaGL com a checagem de erros.

No Vita: instale os dois (são apps separados), feche outros apps, rode cada um
até a tela final e repita pelo menos duas vezes (Cross reinicia, START sai).
Compare em `godot_stderr.log`:

- `[bench] RESULT ...`: `avg_ms`, `p95_ms`, `p99_ms` e `fps` por fase; também
  em `benchmark_gxm.txt` / `benchmark_vitagl.txt` no `user://`;
- `[gxm2d-perf]` x `[vitagl-perf]`: `draws`, `quads`, `submit_us`;
  `gpu_wait_us` (nativo) e `swap_us` (VitaGL) indicam se o quadro é limitado
  pela GPU;
- `[gxm3d] ... submitted_triangles cpu_submit_us`: confira que os dois
  desenharam a mesma quantidade de triângulos antes de comparar tempos;
- `[vita-memory]`.

Confira também na tela: texto, alpha, recortes, a SubViewport (orientação) e
a profundidade das caixas. Os logs não substituem essa inspeção.

Ajustes opcionais em `vitagl.cfg` (passado com `-DVITA_VITAGL_CONFIG=`), uma
chave por linha: `ram_threshold_mb=32`, `cdram_threshold_mb=0`,
`phycont_threshold_mb=8`, `circular_pool_mb=8`, `target_flip=1`.

## Validações feitas

- `tools/tests/test_gxm_bridge_vgl.py`: VitaGL simulada no host. Cobre init
  único, MSAA desligado, `vitagl.cfg`, batching, layout dos vértices,
  cor/tint, origem do scissor, orientação do FBO, RGBA4444 na ordem GL, flush
  antes de liberar textura, restauração do FBO e o caminho 3D (VBO, estado de
  profundidade, triângulos de CPU, passe de luz e volta ao estado 2D).
- Checagem de sintaxe com GCC do host contra o `vitaGL.h` do commit fixado e
  os headers do VitaSDK (`vita-headers`, master), sem avisos com
  `-Wall -Wextra`. Não é uma compilação ARM.
- `test_texture_format.c` continua passando. `test_gxm_bridge.py` já falhava
  antes destas mudanças (fake da libvita2d desatualizado).

Pendentes: compilação ARM, execução no Vita e a medição.
