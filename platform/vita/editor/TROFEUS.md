# Troféus PS Vita no editor

Abra a aba **Troféus PS Vita**, na área inferior do editor. Ela pertence ao projeto aberto e possui três páginas:

1. **Conjunto**: marque **Incluir troféus no VPK**, informe título, descrição e Communication ID. O ID inicial usa o Title ID do primeiro preset Vita + `_00`, mas pode ser editado. Escolha um ID exclusivo e mantenha-o depois de publicar.
2. **Importar Steam**: informe App ID, idioma (`brazilian` ou `english`) e sua chave **Web API de usuário**. Clique **Importar da Steam**. A chave fica apenas na sessão, não é salva no projeto nem no VPK. Também é possível importar o JSON de `GetSchemaForGame/v2`, sem informar uma chave ao editor. Não é um arquivo de conquistas desbloqueadas de um jogador.
3. **Troféus**: edite chave, nome, descrição, categoria, visibilidade e ícone. A Steam não informa Bronze/Prata/Ouro/Platina: novas conquistas começam como Bronze. Reimportar conserva IDs e edições existentes e adiciona apenas chaves novas. Troféus ausentes na nova resposta não são apagados. É possível adicionar manualmente e remover o último item do rascunho, mantendo IDs consecutivos.

**Baixar ícones Steam** usa as URLs do schema e não requer a chave. Ícones indisponíveis recebem arte provisória na geração; podem ser substituídos pela seleção local. Todos os PNG gerados são indexados de 8 bits, com paleta reduzida e transparência binária. Ícone do conjunto: 320 × 176; troféus: 240 × 240.

Salve a configuração e clique **Gerar arquivos base + TRP**. São criados no projeto:

```
vita_trophies/
  trophies.json
  achievement_to_trophy.json
  LEIA-ME.md
  icons/                      # imagens escolhidas/baixadas
  source/
    TROPCONF.SFM
    TROP.SFM
    ICON0.PNG
    TROP000.PNG ...
  vpk_inject/sce_sys/trophy/<COMMUNICATION_ID>/TROPHY.TRP
```

O exportador recria o TRP a partir da configuração salva e o coloca em `sce_sys/trophy/<COMMUNICATION_ID>/TROPHY.TRP` no VPK, tanto no modo com dados separados quanto no modo completo. Os arquivos de edição em `vita_trophies/` não entram nos dados do jogo. Desmarcar **Incluir troféus no VPK** remove o pacote da próxima exportação. A geração não ativa automaticamente essa opção.

A implementação segue o contêiner TRP v2 do exemplo local fornecido: cabeçalho e entradas de 64 bytes, valores big-endian, dados alinhados a 16 bytes e SHA-1 com o campo do hash zerado. O gerador produz SFM não criptografado com marcador de assinatura para o fluxo NoTrpDrm.

## Integração no jogo

O pacote não registra nem desbloqueia troféus sozinho. NoTrpDrm permite usar esse fluxo, mas o jogo ainda precisa inicializar `sceNpTrophy`, registrar o conjunto e desbloquear o ID correspondente quando a condição for satisfeita. **A ponte de desbloqueio nativo ainda não está implementada no runtime Godot deste pacote.** O mapa `achievement_to_trophy.json` fornece os IDs para essa integração futura. Não há desbloqueio automático nem sincronização de conquistas da conta Steam.

O painel, importação por schema JSON, geração, PNG-8, TRP e exportação foram testados no Windows. A consulta online autenticada depende de uma chave do usuário e não foi validada com uma conta real. A compatibilidade final do conjunto e do desbloqueio exige teste no Vita.

Referências:

- [NoTrpDrm](https://github.com/Rinnegatamante/NoTrpDrm)
- [GetSchemaForGame/v2 — Steamworks](https://partner.steamgames.com/doc/webapi/ISteamUserStats#GetSchemaForGame)
