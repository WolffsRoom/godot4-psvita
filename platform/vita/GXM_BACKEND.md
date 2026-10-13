# Backend GXM 2D experimental

## Estado atual

- **Marco 1, aprovado no Vita:** ColorRect visível, cena executando três fases.
  Build gxm2d-colorrect-20261013T020107Z, pacote em
  vpk_out/runtime-gxm-20261012-01/ no workspace. Preservado para regressão.
- **Marco 2, implementado e aguardando hardware:** texturas RGBA, regiões de
  atlas, flips, filtragem, atualização/substituição e fontes bitmap por atlas.
  Cena canvas_features; aplicativo Godot GXM Canvas, GDGX40002.

A referência Ambidextro motivou o caminho GXM; seu repositório não contém
fontes do runtime. Nenhum binário ou asset do jogo foi incorporado.
A avaliação posterior do experimento DeltaruneVita está em
[NATIVE_GXM_EVALUATION.md](NATIVE_GXM_EVALUATION.md).

## Arquitetura

rasterizer_gxm_vita.cpp recebe comandos reais do canvas Godot, aplica
transformações, modulação, UVs e recorte. GXMTextureStorage mantém recursos
por RID e converte imagens para RGBA8. loader/gxm_bridge.c faz upload e
submissão de triângulos via libvita2d, que utiliza GXM e shaders embutidos.
Não há conversão/compilação de shaders GLES3 neste caminho.

A seleção ocorre pelo marcador app0:gxm2d.cfg, antes de inicializar VitaGL.
A interface de inicialização ainda usa a opção Compatibility/opengl3, mas
seleciona outro compositor. O runtime mantém o caminho VitaGL para comparação.
Os serviços ainda não implementados usam classes dummy do Godot; isso não
constitui suporte às respectivas funcionalidades.

A libvita2d usada é a instalada no VitaSDK. O manifesto registra os hashes
do arquivo e do header efetivamente utilizados. Fonte consultada:
https://github.com/xerpi/libvita2d, a8f15ab09d5233f0a4e4ad0e8f6ade0da888cbed.
Não afirmamos que o arquivo do SDK foi compilado desse commit. Licença:
licenses/libvita2d-LICENSE.txt, também incluída no VPK.

## Memória e limites

O backend espera a GPU antes de reutilizar vértices. Texturas descartadas
com um quadro em andamento são liberadas após essa espera. Upload considera
o stride real da textura GXM. Há orçamento de 48 MiB para alocações de
textura, calculadas em páginas de 256 KiB; isso não inclui RAM do engine,
imagens CPU, display buffers ou pools. Ainda não existe expulsão LRU.

Por enquanto: um viewport de tela 960x544, retângulos e texturas 2D. Nearest
e linear estão implementados; mipmaps não. Fontes bitmap usam atlas; MSDF
ou LCD não são suportados. Polígonos gerais, NinePatch, repetição, SubViewport,
luzes, materiais personalizados, efeitos de tela e 3D continuam pendentes.
Escala de viewport, CLEAR_MODE_NEVER e alpha de render targets transparentes
também precisam de implementação/validação próprias.

## Construção

Compile o runtime pelo comando SCons em BUILDING.md. Empacote com os argumentos
usuais de tools/package_runtime.py, acrescentando:

    --backend gxm2d --scene canvas_features

Isso seleciona Godot GXM Canvas (GDGX40002). Para a cena antiga, use
--scene colorrect, que seleciona GDGX40001. VitaGL segue como padrão sem
--backend gxm2d. A cena é empacotada sem depender de assets de jogos comerciais.

## Validações

- Compilação ARM e link do runtime e loader.
- test_gxm_bridge.py: inicialização, geometria/cor, UVs/tint/filter, stride
  de textura NPOT, recorte, liberação adiada e vida útil do pool, em API simulada.
- test_verify_runtime.py: ELF válido e rejeição de casos incompatíveis.
- Cena importada e executada com Godot 4.8-dev7 oficial no host, passando
  pelas fases 0–3 sem erro. O modo headless não valida a imagem.
- Empacotador confere hashes, imports, arquivos do VPK e seleção do backend.

No Vita, confirmar: quatro regiões de atlas, flips, metade semitransparente,
sprite recortado em movimento, rotação, mudança de cor da textura pequena e
texto legível. Logs de envio e FPS não substituem essa inspeção.

A sequência dos demais marcos está em NATIVE_GXM_EVALUATION.md e ROADMAP.md.

## Fallback de fonte no PS Vita (teste 2026-10-12)

O log do build gxm2d-canvas_features-20261013T022616Z confirma boot e fases 0-3, mas registrou 509 erros de fonte.
As aspas vazias do erro FreeType sao geradas por FT_Error_String(error), nao indicam um caminho vazio.
O Godot inclui OpenSans em WOFF2. O VitaSDK FreeType esta sem FT_CONFIG_OPTION_USE_BROTLI,
e FT_Open_Face falha ao tentar decodificar a fonte embutida.

A correcao no scene/theme/default_theme.cpp usa um fallback Lato-Regular.ttf (OFL-1.1)
apenas sob VITA_ENABLED, por meio de platform/vita/fonts/vita_default_font.gen.h.
Demais plataformas mantem a fonte padrao do Godot. A licenca acompanha o VPK.
Fontes WOFF2 customizadas nao sao suportadas pelo FreeType atual e devem ser convertidas
para TTF/OTF ou requerem biblioteca FreeType compilada com suporte Brotli.

A cena canvas_features registra font_height e glyph_width.
O Godot Linux 4.8-dev7 headless reportou font_height=31 e glyph_width=130.
Resultado visual no Vita e ausencia de erros FreeType ainda dependem de novo teste no aparelho.

## Resultado do build de fallback TTF (2026-10-12)

O VPK gxm2d-canvas_features-20261013T025601Z foi empacotado em vpk_out/runtime-gxm-canvas-fontfix-20261012-01/godot_gxm_canvas_fontfix.vpk. O validador ELF retornou PASS com 659 imports. Integridade ZIP, hashes de runtime, licenca de fonte, identidade do backend e cena: PASS. Teste no aparelho ainda pendente.


## Dodge the Creeps: mapeamento de input Vita (2026-10-13)

O projeto original declara Godot 4.7, enquanto os indices do D-pad em Godot 4.8 sao UP=11 DOWN=12 LEFT=13 RIGHT=14. Ajustados na copia tests/dodge_the_creeps/project.godot. Simulacoes de eventos no host confirmaram START, X, analogico e as quatro direcoes. O runtime Vita refeito registra [vita-pad] em stderr para validar sceCtrl antes do InputMap. VPK de teste: vpk_out/runtime-gxm-dodge-controls-20261013-05/dodge_the_creeps_vita_controls.vpk. Teste fisico pendente.
