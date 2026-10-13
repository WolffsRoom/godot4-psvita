# Godot 4.8-dev — PlayStation Vita

Port experimental com VitaGL e loader de `libgodot.so`. O objetivo imediato é
um template 2D funcional. O plano vigente, as evidências e os critérios de
aceite estão em [ROADMAP.md](ROADMAP.md), revisado em 2026-10-10.

## Estado atual

O README principal registra boot, inicialização dos servidores, loop e logo no
Vita. Esses são resultados históricos, não uma certificação do checkout atual.
Há alterações locais de shaders, heap, patch VitaGL e um probe de UBO.

O probe corrigido `ubo-20261010T223545Z` registra **PASS** no Vita para base e
range, com os dois payloads corretos e identidade conferida com o manifesto.
A ponte de `CanvasData` foi implementada com uploads explícitos após o bind
do shader e compilou para Vita. Os três casos do
[Canvas State Probe](tools/canvas_state_probe/README.md) passaram no aparelho,
com pixels exatamente nas cores esperadas e sem erros GL. Outros blocos
flattened ainda exigem tratamento. O canvas 2D completo não está aprovado.

Em 2026-10-10 foi compilado um novo probe identificado, com conferência dos
bytes gravados, e o loader passou na compilação/link usando a mesma biblioteca.
Veja as [instruções de build e teste](tools/ubo_binding_probe/README.md).
O PASS de hardware cobre o probe UBO, não o canvas nem um novo build completo do engine.

Em 2026-10-11, o runtime atualizado foi linkado, validado e empacotado com a
cena ColorRect (`colorrect-20261011T135110Z`, aplicativo `GDCT40001`). Os 650
imports e o conteúdo do VPK passaram nas verificações locais. O teste de
boot e renderização dessa versão no Vita está pendente; veja o ROADMAP.

## Arquitetura e arquivos

| Área | Local | Responsabilidade |
|---|---|---|
| Plataforma | `os_vita.*`, `display_server_vita.*`, `gl_manager_vita.*` | OS, janela fixa, contexto e loop |
| Dispositivos | `audio_driver_vita.*`, `joypad_vita.*` | Áudio e controle; validar no teste integrado |
| Build | `detect.py`, `SCsub` | Toolchain e geração PIC de `bin/libgodot.so` |
| Executável | `loader/` | Carrega `.so`, resolve imports e empacota projeto de teste |
| Renderer | `../../drivers/gles3/` | Compatibility adaptado; tradução ES3 para VitaGL/Cg |
| Compatibilidade GL | `platform_gl.h` | Shims; inclui funções vazias ainda não equivalentes a ES3 |
| VitaGL | `vitagl/` | Patch e notas de dependências |
| Diagnóstico | `tools/` | Probes e análise de dumps |

## Build e validação

Consultar a [receita atual do runtime](BUILDING.md) e as
[notas VitaGL](vitagl/README.md). SCons agora gera `bin/libgodot.so` com
`vita_shared=yes` e `vita_gl_root` explícito. O empacotador
`tools/package_runtime.py` verifica o ELF e os imports e exige o runtime e a
dependência identificados antes de gerar o VPK com a cena ColorRect.
O log antigo de ausência de `pkg-config` não descreve o ambiente atual.

Sequência mínima: biblioteca VitaGL identificada → probe UBO PASS no Vita →
transporte de uniforms validado → ColorRect/textura/Label → runtime interativo.
Para o binding, o teste em `tools/tests/test_vitagl_ubo_binding.py` é apenas
uma guarda de presença da atribuição no código-fonte; não substitui o probe.

## Limites

O port não implementa um driver GLES3 completo. A compilação de shaders e os
probes simples não aprovam todos os recursos do renderer. Atributos inteiros,
bitcasts, interpolação e funções GL vazias exigem validação específica.
3D e GPU particles estão fora do primeiro marco de aceite.

Os relatos históricos de probes e bring-up permanecem no histórico Git.
Para decisões atuais, usar a matriz de evidências e prioridades do ROADMAP.
