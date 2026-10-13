# Avaliação do experimento DeltaruneVita/native-gxm

Fonte examinada, somente leitura:
C:/Users/wolff/Documents/SDKVita/DeltaruneVita/experiments/native-gxm
Hashes dos arquivos consultados: _build/native-gxm-reference-review.json no workspace.

## Conclusão

Útil como referência de engenharia e de testes, não como backend intercambiável
com o Godot. O CMake usa nanovg-gxm com USE_VITA_SHARK=OFF e programas GXP
embutidos. bs_renderer_impl.inc integra a interface do Butterscotch/GameMaker;
nossos pontos de integração são RendererCanvasRender e RendererTextureStorage.
Não copiamos a VM, estruturas TPAG, assets nem o backend NanoVG para Godot.
Não alteramos o projeto DeltaruneVita. Esta avaliação de código não certifica
o comportamento desse experimento no aparelho.

| Referência | Aplicação no Godot |
| --- | --- |
| atlas_probe.c: arte assimétrica e transparência | Nova cena sintética para revelar UV invertida, região errada, filtragem e alpha |
| tpag_sprite.c: recorte, origem, escala e rotação | CommandRect/AtlasTexture do próprio Godot; GameMaker e Godot têm convenções diferentes |
| bs_renderer_impl.inc: cache e orçamento de texturas | Cache por RID, upload sob demanda e teto inicial de 48 MiB para alocações de textura |
| bs_renderer_impl.inc: superfícies e mudança de alvo | Referência para etapa futura de SubViewport; não portada neste marco |
| nanovg_gxm_native.h: shaders GXP embutidos | Mantemos os programas da libvita2d já aprovada no teste ColorRect |
| Diagnósticos explícitos de operação sem suporte | Mantemos avisos para materiais, MSDF/LCD, repetição e comandos ainda ausentes |

## Implementado nesta etapa

- Imagens convertidas para RGBA8, upload respeitando stride/alinhamento GXM.
- Cache por RID, proxies, CanvasTexture difusa e troca/atualização de imagem.
- Liberação adiada até a GPU terminar quando o recurso foi usado no quadro.
- Regiões de atlas, flip H/V, transpose e modulação de cor/alpha.
- Filtro nearest/linear; mipmaps ainda não são gerados/usados.
- Fontes bitmap do Godot usam o caminho de texturas. Validação visual pendente.
- Aceita view_count=0 como estado transitório; avisa apenas multiview >1.
- Cena canvas_features com Sprite2D, AtlasTexture, recorte móvel, rotação,
  transparência, ImageTexture.update/set_image e Labels.

A ponte é testada no host com API simulada: copia linhas de largura não
alinhada corretamente, preserva UV/tint e não libera textura antes da espera
pela GPU. A cena foi importada e executada com Godot 4.8-dev7 oficial,
completando fases 0–3 sem erro. Nada disso substitui o teste visual no Vita.

## Limites importantes

O teto de 48 MiB refere-se às alocações de textura aproximadas em páginas de
256 KiB. Não é o consumo total: imagens CPU, display buffers, runtime e pools
também ocupam memória. Não implementamos LRU: uma alocação acima do limite é
recusada com diagnóstico. Compressão de imagens suportada pelo Godot é
expandida para RGBA8; não equivale a suporte a formatos comprimidos na GPU.

A libvita2d resolve a composição RGB com alpha no framebuffer opaco usado
neste teste. Não declaramos correta a composição de alpha em render targets
transparentes: precisará ser tratada antes de SubViewport.

## Sequência necessária para o port

1. Validar no aparelho atlas, alpha, recorte, filtros, atualização e Label;
   preservar ColorRect como regressão já aprovada.
2. Completar canvas comum: primitivas/polígonos, NinePatch, repetição,
   tamanho lógico de viewport e política de memória medida em cenas reais.
3. Implementar SubViewport, texturas de render target, troca de alvo e clear;
   verificar dependências entre viewports e gerenciamento de recursos.
4. Definir materiais 2D suportados e tratamento explícito de MSDF, luzes,
   efeitos de tela e shaders personalizados na exportação.
5. Consolidar integração do backend (driver próprio, sem nome opengl3),
   exportação/importação de assets, input, áudio, saves e diagnósticos.
6. Avaliar um subconjunto 3D depois do 2D, com testes próprios e orçamento
   de CPU/GPU/memória. O hardware não permite prometer paridade com todos os
   renderizadores e recursos do Godot 4.8.
