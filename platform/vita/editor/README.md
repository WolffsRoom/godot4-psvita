# Editor Windows com exportação PS Vita

A distribuição integrada está em `GodotPSV/editor-vita-integrado`, com entrada
`Godot4Vita.exe` e suporte Vita global. Consulte `INTEGRATED_WINDOWS.md` para
compilação e validação. As instruções abaixo descrevem a alternativa de addon
para o editor oficial.

O complemento `addons/psvita` registra uma plataforma na janela **Projeto >
Exportar > Adicionar > PS Vita** do Godot **4.8-dev7**. Usa a API oficial
EditorExportPlatformExtension. Não requer recompilar o editor, Python, WSL ou
VitaSDK para exportar. O executável do editor continua sendo a distribuição
oficial; a adaptação Vita está no complemento e no template do runtime.

## Usar

1. Copie `addons/psvita` para o projeto, incluindo
   `templates/vita_release.zip` do pacote distribuído.
2. Ative **PS Vita Export** em **Projeto > Configurações do Projeto > Plugins**.
3. Configure Compatibility, viewport 960 x 544 e
   `threading/worker_pool/max_threads=2`. Também é possível usar substituições
   dessas configurações para a feature `Vita`.
4. Adicione PS Vita na janela de exportação. Na aba de scripts, selecione
   **Text (easier debugging)**. O `export_presets.cfg` de exemplo já está pronto.
5. Preencha nome, Title ID (9 letras maiúsculas/dígitos) e versão NN.NN.
   Use um Title ID próprio para não substituir outro jogo instalado.
6. **Exportar Projeto**, escolha `.vpk` O resultado é sempre Release, mesmo com **Export With Debug** marcado.
   Instale o resultado pelo VitaShell no aparelho preparado para este runtime.

O projeto portátil de exemplo vem com plugin ativo, preset, template e uma
textura importada. `Abrir Godot Vita.cmd` abre esse projeto no editor Windows.

## O que é exportado

O próprio Godot seleciona e converte cenas e recursos com `save_zip`. O
exportador coloca esses arquivos na pasta configurada de dados, incluindo
`project.binary` e `.godot/imported`. O runtime lê essa pasta em `ux0:data/`.
Com Data > Separate Folder desativado, os recursos ficam sob `game_data/` no VPK. Não depende de importação de PNG/fontes/áudio no Vita. O addon é removido
do pacote final. EBOOT e libgodot são verificados por SHA-256 antes de empacotar;
o destino só é substituído depois de fechar o VPK temporário com sucesso.

SFO e LiveArea são produzidos sem ferramentas externas. Imagens opcionais:
ícone 128x128, fundo 840x500, botão de início 280x158, em PNG. O template usa
arte geométrica genérica e preserva as licenças das dependências.

## Limitações

Runtime GXM 2D experimental: não há suporte geral a 3D, shaders personalizados,
GPU particles, GDExtension, C# ou depuração remota. Apenas exportação release.
As verificações do preset não certificam compatibilidade de todas as cenas.
A prévia no PC usa o renderer desktop, portanto pode mostrar recursos ausentes
no Vita. Os requisitos do aparelho permanecem os do port (incluindo kubridge).
O template está fixado em 4.8-dev7; não é intercambiável com qualquer versão 4.x.

## Manutenção do template

Compile um loader **genérico**, sem animação/branding de jogo, usando a mesma
libgodot e as dependências correspondentes. Valide com `verify_runtime.py`.
Depois execute:

```
python tools/make_editor_template.py caminho/godot_loader.vpk caminho/vita_release.zip
```

O gerador exclui todos os dados do jogo. Distribua o ZIP dentro de
`addons/psvita/templates`, fora do controle de versão de fontes. Guarde o VPK
original, relatório do verificador e os logs da compilação para rastreabilidade.

## Validação

`test_exporter.gd` exercita a validação de presets e a estrutura do SFO.
`tools/tests/test_editor_vpk.py` verifica o pacote final e extrai uma cópia de
seus recursos para teste no host. O teste final no aparelho continua necessário.

## Padrões e PNG-8

O botão **Aplicar padrão PS Vita** seleciona os arquivos de addons/psvita/defaults sem sobrescrevê-los. Icon, Background e Startup devem ser PNG indexado de 8 bits (paleta), nas dimensões indicadas. Imagens RGB/RGBA de 8 bits por canal não são PNG-8. Referências uid:// escolhidas pelo editor são resolvidas antes da validação.

A opção Export With Debug da janela genérica é aceita, mas VPK, ZIP e PCK são gerados como Release. O exportador informa isso; não adiciona depuração remota.


## Modelo LiveArea

Em Options > Livearea > Style, o padrão é a5. Os 15 estilos documentados no PSDevWiki estão disponíveis. Abra Mostrar prévias dos modelos para ver as miniaturas originais e clique numa delas para selecionar. Aplicar padrão PS Vita também restaura a5. O XML é gerado na exportação e substitui o XML antigo do template. Os modelos especiais do sistema/emulador ainda exigem validação no Vita; as regiões extras mostradas nas prévias não recebem conteúdo automaticamente.

Referência e autoria: https://www.psdevwiki.com/vita/Template.xml. Miniaturas JPEG apenas para documentação do editor; assets LiveArea continuam PNG-8. Licença e atribuição em addons/psvita/previews.


## VPK e dados separados

A exportação sempre gera um `.vpk`, inclusive pela opção Exportar PCK/ZIP (a extensão de saída é ajustada para `.vpk`).
**Data > Separate Folder** vem ativado. **Data > Folder Name** define uma pasta exclusiva do jogo, com 1–64 letras, números, hífen ou sublinhado. Não use caminhos completos nem nomes reservados do Windows.

Exemplo: salve `MeuJogo.vpk` e informe `meujogovita`. Serão gerados lado a lado o VPK e a pasta `meujogovita`. Instale o VPK pelo VitaShell e copie a pasta inteira para `ux0:data/`, resultando em `ux0:data/meujogovita/project.binary` e os demais recursos. Preserve arquivos ocultos e a pasta `.godot` exportada. O nome diferencia maiúsculas e minúsculas; mantenha a grafia exata. O VPK ainda inclui o runtime, a animação e os assets LiveArea.

Desmarque **Separate Folder** para incluir todos os dados dentro do VPK, sem cópia separada no Vita. Um template antigo sem suporte a dados externos só permite esse modo completo.

Ao reexportar, a pasta anterior gerada para o mesmo Title ID é preservada com sufixo `.previous-...`; pode ser removida após conferir a nova exportação. Uma pasta preexistente sem identificação do exportador não é substituída. Pastas `.partial-...` indicam uma tentativa incompleta e não devem ser copiadas para o Vita.
