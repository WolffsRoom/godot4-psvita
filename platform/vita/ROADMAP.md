# Plano do port Godot 4.8-dev para PS Vita

Revisão local: 2026-10-10. Base do engine: `cdf4eec`, com alterações locais
anteriores à revisão. `version.py` identifica 4.8-dev; isto não declara suporte
completo a Godot 4.8. Esta revisão organiza o trabalho, sem certificar um novo
build ou uma nova execução no console.

## Objetivo imediato

Entregar um template experimental 2D: iniciar pelo loader, executar GDScript,
desenhar ColorRect, textura e Label corretamente e responder ao controle.
3D, editor no aparelho, GPU particles e paridade completa ficam para depois
desse marco. Compilar shaders não basta para declarar renderização funcional.

## Avanço de implementação — 2026-10-10

- Base VitaGL fixada em `vitagl/dependency.json` (`df63ce8211ce...`). O patch
  aplica na base limpa com finais de linha LF; as falhas anteriores não
  demonstravam incompatibilidade dessa base. O build normaliza o patch.
- `tools/build_ubo_probe.py` gera uma cópia limpa, aplica o patch e compila
  biblioteca e probe, sem alterar o checkout de origem ou instalar no SDK.
  Gera manifesto com hashes, opções e comando de link.
- Probe e loader agora usam `cmake/VitaGL.cmake`: biblioteca e headers
  explícitos por `VITA_GL_ROOT`. Os caminhos `/tmp` foram removidos do CMake
  do loader; a integração SCons ainda está pendente.
- Probe reforçado com conferência dos bytes gravados e identidade do build
  no log. VPK compilado: `ubo-20261010T222928Z`, SHA-256
  `53978edef6c44dedd0ce430084a8235fa3ce6a2d0a8b36cc51ca4ca0f9c94ed4`.
- Verificações locais passaram: patch + guarda de fonte, integridade e
  identidade do VPK, recusa de VitaGL não especificada e compilação/link do
  executável loader com a mesma biblioteca. A guarda rejeita a base sem patch.
  O loader ainda emite avisos de declarações em `default_dynlib.c` e de stack
  executável no objeto `crtn.o` do SDK; não foram tratados neste passo.
- `pkg-config` está disponível no WSL atual. O log antigo de sua ausência
  continua como histórico; não foi feito novo build completo do engine.

**Gate atual:** executar o runtime ColorRect `colorrect-20261011T135110Z`
no Vita e confirmar a cena real. O VPK já foi gerado e validado localmente;
os probes UBO e CanvasData passaram no aparelho, conforme os registros abaixo.
Os achados abaixo registram a revisão inicial; este avanço resolve a seleção
da dependência para probe/loader, mas não o transporte UBO → shader do canvas.

### Retorno do aparelho e correção do probe

O log recebido para `ubo-20261010T222928Z` corresponde ao hash da biblioteca,
mas termina em `FAIL (VitaGL initialization)` antes dos testes UBO. A causa
identificada é uma verificação incorreta introduzida no probe: na base fixada,
`vglInit()` retorna a flag de fallback de resolução, não sucesso/falha.
`GL_FALSE` é esperado quando não precisa reduzir a resolução. A condição de
saída foi removida; o valor passa a ser registrado apenas como diagnóstico.
O build anterior fica invalidado como teste UBO e precisa ser substituído.
Sua evidência foi preservada em `vpk_out/ubo-20261010-01/device-log.txt` e
`hardware-result.json` no workspace. O aceite de hardware continua pendente.

Build corrigido: `ubo-20261010T223545Z`, entregue em
`vpk_out/ubo-20261010-02/ubo_binding_probe.vpk` no workspace. SHA-256:
`835aafc4cf2cf5d7b7f326abf8d3af6ccd6b0e3d052fff1b3d0af747996abbce`.
Compilação, integridade do pacote, hashes dos inputs e identidade embutida
verificados. A mensagem de falha incorreta não consta do executável novo.

### Aceite UBO no aparelho — 2026-10-10

Log recebido de `ubo-20261010T223545Z`: **base=PASS, range=PASS, RESULT: PASS**.
Os dois payloads retornaram `match=1`, sem erros no upload, mapeamento ou
desmapeamento. O ID e o SHA-256 da VitaGL coincidem com o manifesto do build.
Evidências preservadas em `vpk_out/ubo-20261010-02/device-log.txt` e
`hardware-result.json`. A etapa 2 está concluída para essa biblioteca.

Isso valida o binding genérico e os bytes enviados aos buffers, não a leitura
dos dados pelo shader. A próxima etapa é reconciliar os uniforms individuais
gerados pelo down-emitter com os uploads UBO do canvas e provar um ColorRect
com transformação e cor conhecidas.

## Estado e evidências

| Área | Evidência local | Situação |
|---|---|---|
| Boot, servidores, loop e logo | Relato no README principal e binários existentes | Histórico; repetir com o próximo build identificado |
| API GL básica | `../../../vpk_out/es3_probe.txt`: VAO, binding UBO, map e instancing PASS | Testes parciais; log termina no link GLSL 300 |
| Shader simples float/int | `../../../vpk_out/dialect_probe.txt`: link e pixel esperado PASS | Não valida atributos inteiros, batching ou o canvas completo |
| Binding genérico UBO | `../../../Logs/ubo_binding_probe.txt`: base/range FAIL, erro `0x0502` | Última evidência desse probe é falha |
| Correção UBO | Patch VitaGL e cópia local contêm atualização de `uniform_array_unit` | Implementada em fonte; sem novo PASS do aparelho identificado |
| Build reproduzível | `../../../_build/godot4-scons.log`: falta `pkg-config` | Essa tentativa parou na configuração; não é erro de compilação C++ |
| Canvas completo / 3D | Sem aceite identificado nesta revisão | Em aberto |

Os caminhos externos acima pressupõem o workspace original; esses logs não
fazem parte necessariamente do repositório distribuído.

## Achados prioritários da revisão

1. **P0 — reconciliar uniforms e UBOs.** `_vita_downemit()` em
   `drivers/gles3/shader_gles3.cpp` remove blocos e cria uniforms individuais.
   O renderer ainda faz `glBindBufferBase` + `glBufferData` para `StateBuffer`
   em `rasterizer_canvas_gles3.cpp`, e o shader tenta localizar os blocos com
   `glGetUniformBlockIndex`. Corrigir o binding genérico é necessário para o
   upload, mas não demonstra que os uniforms individuais recebem os dados.
   Escolher e validar um contrato único: preservar blocos compatíveis com a
   tradução VitaGL, ou enviar explicitamente os uniforms após o flattening.
   O caminho atual apresenta incompatibilidade estática; confirmar o efeito
   com matrizes e cores conhecidas antes de atribuir toda tela cinza ao binding.
2. **P0 — identificar a biblioteca realmente vinculada.** O loader usa
   `/tmp/vgl/libvitaGL.a`; o probe usa `vitaGL` por nome. É possível testar e
   executar bibliotecas diferentes. O README do patch cita `ed51b21`, enquanto
   `_build/vitaGL-clean` está em `df63ce8` e possui modificações locais.
   `git apply --check` falha nessa árvore modificada; isso não prova defeito do
   patch em sua base original. Validar em checkout limpo da base declarada.
3. **P1 — o build documentado ainda não fecha a cadeia.** `SCsub` produz um
   executável e passa por ELF/SELF; o loader espera `loader/libgodot.so`.
   Falta um procedimento integrado e verificável para relink PIC, imports e
   empacotamento. `detect.py` e o CMake do loader dependem de `/tmp`.
4. **P1 — conversão de shader ainda tem perdas semânticas.**
   `floatBitsToUint`/`floatBitsToInt` retornam zero; `flat` é removido;
   `uint` é remapeado e atributos inteiros passam por `glVertexAttribPointer`.
   Validar bits, flags, cores compactadas e limites numéricos. Sucesso do
   compilador não garante equivalência. Cobrir também assinaturas multilinha
   e condicionais na heurística de substituição de `in/out`.
5. **P1 — orçamento de memória tem duas definições.** O engine declara 96 MiB
   em `godot_vita.cpp`; o executável loader declara 128 MiB em `loader/main.c`.
   Medir o heap efetivo e os pools GXM no caminho `.so`; a alteração do engine
   isoladamente não comprova mudança do heap inicializado pelo executável.
6. **P2 — funções GL vazias precisam virar capacidades explícitas.**
   `platform_gl.h` contém no-ops como `glTexStorage2D`, `glClearBufferfv` e
   transform feedback. Auditar chamadas alcançáveis pelo 2D e implementar o
   subconjunto necessário ou rejeitar o recurso de forma clara.

## Etapas e critérios de conclusão

| Ordem | Trabalho | Critério de conclusão |
|---|---|---|
| 1 | Fixar base VitaGL, patch, SDK e bibliotecas; resolver `pkg-config`; registrar alterações locais | Patch aplica em base limpa; versões, opções e hashes registrados; fonte do probe e loader coincide |
| 2 | Recompilar e rodar `ubo_binding_probe` com a biblioteca corrigida | Novo log no Vita com base=PASS, range=PASS e RESULT: PASS |
| 3 | Validar transporte CPU → shader do canvas | Transformação, modulação e tamanho de tela conhecidos chegam ao shader; retângulo aparece na posição e cor esperadas |
| 4 | Corrigir atributos e variantes mínimas de canvas | ColorRect animado, textura com alpha, Label, clipping, escala/rotação e múltiplos batches corretos no aparelho |
| 5 | Automatizar engine PIC → loader → VPK | Build em ambiente preparado sem passos manuais ocultos; imports resolvidos; manifesto identifica todos os artefatos |
| 6 | Consolidar runtime 2D | Cena interativa com botões/analógicos, áudio e gravação em `user://`; execução de 10 minutos sem crash, com FPS e memória registrados |
| 7 | Publicar preview 2D e só então avaliar 3D | VPK, instruções, limitações, hashes e evidências correspondem ao mesmo build |

A etapa 5 pode ser preparada junto das etapas 2–4; a identificação dos
artefatos da etapa 1 é obrigatória antes de interpretar os resultados.

## Organização do trabalho

- `platform/vita/README.md`: entrada técnica e estado resumido.
- Este arquivo: prioridades, decisões pendentes e critérios de aceite.
- `platform/vita/vitagl/`: patch e contrato de dependência VitaGL.
- `platform/vita/tools/`: probes isolados; `loader/game_data/`: teste integrado.
- `godot-vita-3.x` no workspace: referência histórica, sem misturar builds.
- `_build`, `Logs` e `vpk_out` no workspace: preservar evidências existentes.
  Para novas execuções, separar por identificador de build, sem renomear
  retroativamente arquivos cuja origem ainda não está comprovada.

Separar próximos commits por dependência: patch/probe UBO, transporte de
uniforms, atributos/shaders, build/empacotamento e documentação. As mudanças
locais de shader, heap, patch e probes já existentes devem ser revisadas antes
de entrar nesses commits; não foram descartadas por esta revisão.

## Registro obrigatório para cada teste no aparelho

Guardar data, modelo do Vita, versões das dependências de runtime, commit do
engine e diff local, commit/patch VitaGL, versão do SDK, opções de compilação,
SHA-256 do VPK e `.so`, cenário executado, resultado esperado/observado,
logs e captura visual. Em crash, guardar também o dump e o ELF com símbolos
correspondente. Um resultado de probe isolado não aprova o engine completo.

## Próxima sessão

Executar o VPK do runtime ColorRect descrito abaixo, coletar os logs e
confirmar visualmente o ColorRect com o shader real e os atributos do canvas.
Não avançar para otimização ou 3D enquanto esse aceite estiver aberto.

## Implementação do transporte CanvasData — 2026-10-10

O renderer agora preserva o `StateBuffer` da passagem corrente e, após bind
bem-sucedido de cada shader de batch, envia explicitamente os uniforms que o
down-emitter extraiu de `CanvasData`. A função compartilhada está em
`canvas_uniforms_vita.h`, protegida por `VITA_ENABLED` na integração do engine.
Uploads UBO existentes foram mantidos; a ponte cobre somente `CanvasData`.

A unidade `rasterizer_canvas_gles3.cpp` compilou para ARM/Vita com a integração,
sem diagnósticos. Foi uma compilação isolada, não um novo link do engine.
O probe `tools/canvas_state_probe` compilou e usa a mesma função de upload e a
mesma biblioteca VitaGL que passou no probe UBO. Ele verifica pixels de três
retângulos com transformações, cores, programas e estados diferentes.

Build: `canvas-state-20261010T224441Z`. Pacote no workspace:
`vpk_out/canvas-state-20261010-01/canvas_state_probe.vpk`.
O aceite no aparelho está registrado abaixo. PASS desse probe valida a ponte
isolada; não aprova o shader completo, batching, Label ou texturas do Godot.

### Aceite CanvasData no aparelho — 2026-10-10

Build `canvas-state-20261010T224441Z`, identidade e hash VitaGL conferidos:
**três casos PASS, RESULT: PASS**, sem erros GL. As amostras internas coincidem
exatamente com os RGB esperados: (255,64,64), (64,255,64) e (128,128,255).
As três amostras externas retornaram preto, também como esperado.

Isso confirma no probe as matrizes, modulação, pixel size, time, boolean e
atualização de estado após trocar/reutilizar programas. Logs preservados em
`vpk_out/canvas-state-20261010-01/device-log.txt` e `hardware-result.json` no
workspace. Campos normal/SDF/luz e o canvas completo seguem sem aceite.

## Runtime ColorRect entregue — 2026-10-11

- SCons gera `bin/libgodot.so` diretamente com `vita_shared=yes`, usando
  `vita_gl_root` explícito. O link agrupa as bibliotecas do engine e inclui
  seus objetos necessários; não força objetos auxiliares não usados do HarfBuzz.
- Build e link concluídos. O novo runtime passou na verificação de ARM
  hard-float, segmentos RX/RW, relocações, entry point e **650 imports**.
  O validador também rejeitou fixtures com import ausente, arquitetura errada
  e relocação não suportada.
- O loader agora exige caminho explícito do runtime, registra os hashes e
  interrompe a inicialização quando algum import não pode ser resolvido.
- O empacotador usa a VitaGL já aprovada no aparelho e a cena
  `tests/colorrect`, sem Label ou assets importados. Confere o conteúdo do
  VPK contra runtime, eboot, cena e title ID esperados.
- Símbolos não necessários foram removidos apenas da cópia empacotada:
  65.225.876 → 45.920.856 bytes. O ELF reduzido foi validado novamente; a
  versão completa foi preservada em
  `_build/runtime-colorrect-20261011-02/libgodot.unstripped.so` no workspace.

**Pacote:** `vpk_out/runtime-colorrect-20261011-02/godot_colorrect.vpk` no
workspace. Aplicativo **Godot ColorRect Test**, title ID `GDCT40001`.
Build ID: `colorrect-20261011T135110Z`.
SHA-256 VPK: `89f2ac178d96f47d13db25d43b8c92bc2deaae630ee1f26fcb4949277a8cf207`.

Critério de aceite: fundo preto, retângulo vermelho à esquerda, verde à
direita e roxo menor abaixo, alternando a cada três segundos sem crash.
Logs de GDScript comprovam a execução da cena, não os pixels; confirmar
visualmente e coletar os logs indicados em [BUILDING.md](BUILDING.md).
**Boot e renderização deste novo runtime ainda não foram testados no aparelho.**

## Retorno do aparelho e correção — 2026-10-11

O build `colorrect-20261011T135110Z` exibiu a logo Godot e permaneceu preto.
Os hashes do loader conferem. Setup e start terminaram, mas faltam registros
que comprovem a execução da cena e a conclusão do primeiro quadro.
Logs originais preservados em
`vpk_out/runtime-colorrect-20261011-02/device-logs/` no workspace.

Defeito identificado no conversor de shaders: a contagem de chaves somava
os dois loops alternativos de `s4ao_inc.glsl`, mantendo profundidade falsa
após a função. Com isso, `in vec2 uv_interp` não virava `varying` e o shader
Post falhava por ausência de semântica. O conversor agora restaura a
profundidade ao entrar em `#else`/`#elif`, inclusive com condicionais aninhadas.
Regressão de host executa o trecho C++ de produção com o include real de SSAO
e confirma que parâmetros `in`/`out` de funções continuam intactos.

Próximo pacote inclui registros dos três primeiros quadros, READY/fases da
cena em stderr e cópias do último shader de cada estágio antes da compilação.
Esses registros são temporários de diagnóstico, sem teste de desempenho.
Ainda pendentes: confirmar Post no aparelho, localizar os erros fatais finais
do compilador e tratar texturas/framebuffers não suportados e dados do canvas.
Os erros SceneShader 3D continuam pendentes; não equivalem a aceite do 2D.

Pacote de correção/diagnóstico concluído:
`vpk_out/runtime-colorrect-20261011-03/godot_colorrect.vpk`.
Build ID `colorrect-20261011T185837Z`.
SHA-256 VPK `d194cc088d3445d630558b674120194c5502272b0692b717585990ff4fd334af`.
Compilação, regressão, ELF/imports e conteúdo do VPK aprovados localmente.
**Aguardando teste no Vita; tela preta ainda não considerada resolvida.**

## Retorno do aparelho — 2026-10-12

Build `colorrect-20261011T185837Z` confirmado pelos hashes. Evidência em
`vpk_out/runtime-colorrect-20261011-03/device-logs/` e `hardware-result.json`.

- PostShader: ambos os estágios chegaram ao link, que retornou sem erro
  registrado. A falha anterior de semântica de uv_interp não reapareceu.
- Cena: READY confirmado; fases 0, 1 e 2 repetidas com posições/tamanhos
  esperados; primeiros três quadros concluídos; loop reportando 59–60 FPS.
- Bloqueio atual: CanvasShaderGLES3 falha duas vezes no estágio de vértices
  com `fatal internal error on line -1`. Não chega ao estágio de fragmentos.
- O arquivo godot_shader_fragment.glsl é de um shader anterior; não pode
  ser tratado como o par do vertex dump do Canvas.
- Erros SceneShader 3D persistem. Nenhuma nova confirmação visual recebida.

Próxima etapa prioritária: reproduzir o vertex dump do Canvas em um probe
isolado com a mesma VitaGL; reduzir o shader por variantes para distinguir
falha do tradutor e limitação do compilador. O log não identifica a instrução
responsável, portanto ainda não atribuímos a causa a um builtin específico.
Depois de compilar, validar os atributos inteiros e os dados por instância:
platform_gl.h ainda encaminha glVertexAttribIPointer para o caminho de
atributos de ponto flutuante. Só então avaliar os pixels e os formatos de
framebuffer. Não avançar para 3D nem considerar o 2D aprovado pelo FPS.

## Nova direção: canvas GXM nativo — 2026-10-12

Por solicitação do usuário, usar o Ambidextro como referência de abordagem.
A fonte do seu runtime não está no repositório indicado e o usuário não tem
outro link. Implementamos um backend próprio mínimo, sobre libvita2d/GXM,
sem reaproveitar o binário ou os assets de Ambidextro.

Implementação, limites, testes e próximos marcos em [GXM_BACKEND.md](GXM_BACKEND.md).
Primeiro objetivo: nossa cena ColorRect rodando pelo canvas real do Godot,
sem compilar os shaders GLES3 que falhavam. Title ID separado: GDGX40001.
O caminho VitaGL é preservado para comparação. Ainda depende de teste no Vita.

Pacote GXM inicial entregue em
`vpk_out/runtime-gxm-20261012-01/godot_gxm_colorrect.vpk`.
Build ID: `gxm2d-colorrect-20261013T020107Z` (identificador UTC do empacotador).
SHA-256: `0908d0713aede2cfd458f521d1f2c2f4242f88134c502241ffc688c7aebfe2d2`.
Compilação ARM/runtime/loader, 656 imports, teste da ponte GXM com API simulada,
rejeições do validador ELF e integridade do pacote: PASS.
**Aceite visual no Vita pendente. Não representa suporte geral a Godot 4.8.**

## Resultado no Vita: ColorRect visível

Build `gxm2d-colorrect-20261013T020107Z` e hash do runtime confirmados nos logs.
O usuário informou: **Exibiu quadrados coloridos.**

- `vita2d_init=1` e backend nativo GXM ativo.
- READY, primeiros três quadros completos e fases 0/1/2 repetidas.
- Loop registra 59–60 FPS; nenhuma falha de compilação CanvasShaderGLES3.
- Aceite do marco inicial: **retângulos coloridos visíveis no aparelho**.
  Geometria e cores exatas ainda não foram medidas por captura/readback.
- Persistem mensagens de caminho do executável, cache global de scripts
  ausente e aviso de quantidade de views; revisar sem perder esta base funcional.

Evidência preservada em `vpk_out/runtime-gxm-20261012-01/device-logs/`
e `hardware-result.json`. O pacote e o runtime testados permanecem arquivados.
Próximo marco: textura RGBA/sprite, transparência e recorte; depois fontes/Label.
Isto não valida shaders personalizados, render-to-texture ou 3D.

## Avaliação do native-gxm e segundo marco do canvas

O experimento DeltaruneVita foi examinado em modo somente leitura. É útil
para testes de atlas, conversão UV, sincronização, orçamento de texturas e
futura implementação de superfícies. Não copiamos o adaptador GameMaker/NanoVG:
integramos os conceitos à interface de canvas e texturas do Godot.
Avaliação completa: [NATIVE_GXM_EVALUATION.md](NATIVE_GXM_EVALUATION.md).

Implementado: RGBA8 com stride nativo, cache por RID, proxies/CanvasTexture,
atlas/flip/transpose, filtro nearest/linear, tint/alpha, atualização e
substituição de ImageTexture, descarte após GPU e limite de 48 MiB.
Fontes bitmap usam esse mesmo caminho de atlas; teste visual pendente.

Nova cena canvas_features cobre texturas assimétricas, recorte, rotação,
transparência, Labels e atualização/substituição de textura NPOT 13x9.
Godot 4.8-dev7 oficial importou e executou fases 0–3 sem erros no host.
Ponte gráfica passou em testes simulados e runtime ARM compilou.
O teste ColorRect aprovado permanece intacto em seu pacote anterior.

Próxima decisão depende do resultado visual deste pacote no Vita. Depois:
primitivas/NinePatch/repetição, SubViewport e recursos de canvas, materiais,
exportação e integração da plataforma; 3D exige uma etapa independente.
