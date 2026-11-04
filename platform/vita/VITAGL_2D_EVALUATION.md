# VitaGL no canvas 2D: backend A/B e avaliação de desempenho

Revisão: 2026-11-04. Estado: **implementado e testado no host com API
simulada; não compilado para ARM e não executado no Vita.** Nenhum número de
desempenho deste documento foi medido no aparelho.

## O que existe hoje

| Caminho | Uso atual | Situação |
|---|---|---|
| Canvas GXM nativo (`gxm_canvas_vita.inc` → `loader/gxm_bridge.c` → libvita2d/GXM) | Todos os jogos 2D e a UI dos jogos 3D | Em produção, 60 FPS reportados em jogos 2D |
| Protótipo 3D GXM (`rasterizer_scene_gxm_vita.inc`) | Overburden, Buckshot Roulette | Usa os programas da libvita2d com MVP real e malhas residentes na GPU |
| Compatibility GLES3 sobre VitaGL (`drivers/gles3` + `gl_manager_vita.cpp`) | Nenhum jogo | Parado em 2026-10-12: `CanvasShaderGLES3` falha no compilador com `fatal internal error on line -1` |

Ou seja, o 2D e o 3D atuais **não usam VitaGL**. VitaGL continua vinculada ao
loader só porque o `libgodot.so` também importa as funções `gl*` do caminho
GLES3.

## O que foi implementado

`loader/gxm_bridge_vgl.c` implementa o mesmo contrato `vita_gxm_*` da ponte
nativa, sobre VitaGL. O `libgodot.so` e todo o código Godot do canvas ficam
idênticos; só muda o `eboot.bin`. Essa é a comparação mais justa possível:
mesmo jogo, mesmos comandos de canvas, mesmo batching, apenas outro transporte
até a GPU.

- Dois programas GLSL ES 1.00 mínimos (cor e textura com tint), compilados uma
  vez no init pelo compilador de VitaGL. Não usa os shaders GLES3 do Godot.
- Batching igual ao nativo: quads consecutivos com a mesma textura, filtro e
  tint viram um draw; quads de cor sólida sempre se juntam.
- Recorte por `glScissor`, SubViewport por FBO, blend igual ao da libvita2d
  (`SRC_ALPHA/ONE_MINUS_SRC_ALPHA`, aditivo `ONE/ONE`), RGBA8 e RGBA4444.
- Sem MSAA (a libvita2d também não usa; o `gl_manager_vita.cpp` usa 4x).
- Estatísticas `[vgl2d-perf]` com os mesmos campos de `[gxm2d-perf]`.
- A ponte nativa ganhou o campo `submit_us` no fim da linha `[gxm2d-perf]`
  (tempo de CPU dentro da submissão dos draws), para comparar diretamente
  com o `submit_us` de VitaGL. Os campos anteriores não mudaram de ordem.

Limites deste backend:

- Só 2D. As entradas do protótipo 3D retornam "indisponível" e o init recusa
  pacotes com `gxm3d.cfg`, em vez de desenhar errado. Tilt-shift cai no quadro
  normal.
- Exige `libshacccg.suprx` no console (a ponte nativa não exige).
- Se a compilação dos shaders falhar depois do `vglInit`, não há fallback para
  libvita2d: o GXM já pertence a VitaGL. O log informa a causa.
- A orientação de FBO segue a convenção GL da build sem
  `HAVE_UNFLIPPED_FBOS`, conforme o código de `framebuffers.c`/`misc.c` do
  commit fixado. Se a SubViewport aparecer invertida no aparelho, use
  `target_flip=0` no `vgl2d.cfg`.

## Avaliação: deve haver ganho?

**Hipótese, baseada na leitura do código da libvita2d e da VitaGL do commit
`df63ce8`: não, para o 2D atual o esperado é empate na GPU e mais custo de CPU
e memória com VitaGL.** A medição A/B abaixo confirma ou derruba isso.

### CPU por draw

A ponte nativa já escreve os vértices diretamente na pool mapeada da GPU, e
cada draw da libvita2d é: programas, textura, reserva do uniform buffer com a
matriz e o tint, vertex stream e `sceGxmDraw`.

Em VitaGL, o mesmo draw (`glDrawArrays` → `_glDrawArrays_CustomShadersIMPL`)
faz, além disso:

1. Copia os vértices client-side para o circular pool
   (`gpu_alloc_mapped_temp` + `vgl_fast_memcpy`). Os vértices são escritos duas
   vezes: na área de staging e na memória da GPU.
2. Valida o estado sujo (blend, scissor, viewport) e procura o fragment
   program pelo estado de blend.
3. Copia todos os uniforms para o uniform buffer reservado.
4. Na build fixada (`NO_DEBUG=0`, `LOG_ERRORS=2`), mantém a checagem de erros
   GL em cada chamada.

O número de draws é igual nos dois caminhos, porque o batching é o mesmo.
Então a diferença aparece em `submit_us` e cresce com a quantidade de draws e
de quads (tilemaps, texto).

### GPU

Mesma SGX543, mesma resolução, mesmos shaders triviais e mesma quantidade de
fragmentos. Em cenas limitadas por fill rate (camadas de sprites em tela
cheia), não se espera diferença. O recorte também não favorece VitaGL: a
libvita2d desenha dois retângulos de stencil por mudança de clip, e VitaGL
desenha um quad de profundidade (`update_scissor_test`) mais o region clip
por tile.

### Memória

VitaGL reserva no init quase toda a memória livre de cada pool (menos os
limites configurados) e cria um circular pool, de 32 MiB por padrão. A
libvita2d aloca textura por textura e usa uma pool de vértices de 4 MiB. O
backend A/B usa por padrão 8 MiB de circular pool e deixa 32 MiB de USER_RW e
8 MiB de phycont livres para pilhas de threads e alocações do kernel feitas
depois do renderer. Jogos no limite (Buckshot Roulette) tendem a sofrer mais
com VitaGL.

### 3D

O protótipo 3D já mantém as malhas na GPU e usa programas GXM próprios.
Passar o 3D para VitaGL adicionaria o mesmo custo por draw, e os shaders de
cena GLES3 do Godot já falharam no compilador. Um ganho real no 3D viria de
shaders GXM dedicados (por exemplo, iluminação por vértice na GPU), e isso
não depende de VitaGL.

### Onde VitaGL ajuda de verdade

Nas funcionalidades, não no desempenho. VitaGL compila GLSL em tempo de
execução, o que abriria caminho para shaders de `CanvasItem`/materiais
personalizados. Isso não está neste backend: exigiria traduzir shaders Godot
e aceitar a dependência do `libshacccg.suprx`. Se esse for o objetivo, o
backend A/B é a base certa para crescer, ao lado do caminho nativo.

## Como medir (A/B)

1. Use o `libgodot.so` atual, sem recompilar.
2. Compile dois loaders com o mesmo `VITA_RUNTIME` e o mesmo jogo, sem
   `VITA_GXM3D`, com Title IDs diferentes:

   ```sh
   cmake -S platform/vita/loader -B build-native -DVITA_GXM2D=ON ...
   cmake -S platform/vita/loader -B build-vgl -DVITA_VGL2D=ON \
     -DVITA_TITLEID=GDVG00001 ...
   ```

   Para não penalizar VitaGL com a checagem de erros, prefira uma VitaGL do
   mesmo commit e patch, compilada com `NO_DEBUG=1` e sem `LOG_ERRORS`
   (registre o SHA-256 que o loader imprime). Repita com a build de debug se
   quiser medir o custo da checagem.
3. Alternativa para um jogo já exportado: copie o VPK, troque o `eboot.bin`
   pelo do `build-vgl`, adicione `vgl2d.cfg` (opcional) e remova `gxm3d.cfg`.
   Remova `gxm3d.cfg` também do VPK de referência, para os dois rodarem o
   mesmo renderer 2D.
4. Rode a mesma cena por pelo menos 3 blocos de 300 quadros e compare em
   `godot_stderr.log`:
   - `avg_frame_us`, `fps`, `draws`, `quads`, `clips`, `submit_us`;
   - `gpu_wait_us` (nativo) e `swap_us` (VitaGL) indicam se o quadro é
     limitado pela GPU;
   - as linhas `[vita-memory]`.
5. Confira na tela: texto, alpha, recortes, SubViewport (orientação) e
   texturas RGBA4444. Os logs não substituem essa inspeção.

Ajustes opcionais em `vgl2d.cfg`, uma chave por linha:
`ram_threshold_mb=32`, `cdram_threshold_mb=0`, `phycont_threshold_mb=8`,
`circular_pool_mb=8`, `target_flip=1`.

## Validações feitas

- `tools/tests/test_gxm_bridge_vgl.py`: VitaGL simulada no host. Cobre recusa
  com `gxm3d.cfg`, init único, MSAA desligado, leitura do `vgl2d.cfg`,
  batching, layout dos vértices, conversão de cor/tint, origem do scissor,
  orientação do FBO, empacotamento RGBA4444 na ordem GL, flush antes de
  liberar textura e restauração do FBO ao criar textura no meio de um alvo.
- Checagem de sintaxe com GCC do host contra o `vitaGL.h` do commit fixado e
  os headers do VitaSDK (`vita-headers`, master): `gxm_bridge_vgl.c` e
  `gxm_bridge.c` sem avisos com `-Wall -Wextra`. Não é uma compilação ARM.
- `test_texture_format.c` continua passando.
- `test_gxm_bridge.py` já falhava antes desta mudança (o fake da libvita2d
  está desatualizado em relação à ponte atual).

Pendentes: compilação ARM do loader com `VITA_VGL2D=ON`, execução no Vita e a
medição A/B.
