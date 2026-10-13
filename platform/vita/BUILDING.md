# Runtime ColorRect: build e empacotamento

Requer Linux/WSL, Python 3.12+, SCons (validado com 4.9.1), CMake, Make,
pkg-config e VitaSDK. O SDK usado aqui tem GCC 15.2.0 e as bibliotecas de
VitaGL, codecs e stubs referenciadas em `loader/CMakeLists.txt`.

Use a biblioteca do build UBO aprovado, não uma cópia desconhecida instalada
no SDK. Para gerar essa dependência, consulte
[`tools/ubo_binding_probe/README.md`](tools/ubo_binding_probe/README.md).

## 1. Compilar o engine

A partir do diretório `godot-master`:

```sh
export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"
python3 -m SCons platform=vita arch=arm32 target=template_release \
  optimize=size disable_path_overrides=no vita_shared=yes \
  vita_gl_root=/absolute/path/to/ubo-build/vitagl -j6
```

Isso produz `bin/libgodot.so` (PIC, ARM hard-float). Os arquivos estáticos do
engine são vinculados ao `.so`, e dependências externas permanecem como
imports resolvidos pelo loader. O caminho legado ELF/SELF só é selecionado
explicitamente com `vita_shared=no` e não é o caminho validado para este teste.
`vita_gl_root` é obrigatório e substitui a antiga seleção implícita de `/tmp`.

Se SCons não estiver instalado, pode ser mantido somente no workspace:

```sh
python3 -m pip install --target /absolute/path/to/python-tools scons==4.9.1
export PYTHONPATH=/absolute/path/to/python-tools
```

## 2. Validar e empacotar

Ainda em `godot-master`, usando uma pasta de saída nova:

```sh
python3 platform/vita/tools/package_runtime.py \
  --runtime bin/libgodot.so \
  --ubo-build /absolute/path/to/ubo-build \
  --output /absolute/path/to/runtime-new-build
```

O empacotador confere identidade da biblioteca, formato ELF, tipos de
relocação, entry point e todos os imports contra `default_dynlib.c`. Em
seguida compila o loader e empacota o `.so` explícito com
`tests/colorrect`. Não usa automaticamente `loader/libgodot.so`.

O resultado inclui `godot_colorrect.vpk`, manifesto, relatório de validação,
log, cópia do `.so` e ELF do loader. O manifesto identifica os inputs de
empacotamento e os arquivos principais da ponte canvas; não é uma cópia de
todo o SDK. Preserve também o log SCons e o checkout usado para depuração.

O `.so` empacotado passa por `strip --strip-unneeded`, seguido de nova
validação do contrato ELF. O original `bin/libgodot.so` permanece completo
para depuração; seu caminho, tamanho e hash também constam no manifesto.
Isso reduz a memória temporária usada pelo loader, que lê todo o arquivo
antes de mapear os segmentos. Preserve o original correspondente ao VPK.

A verificação do VPK compara o `.so`, eboot, arquivos da cena e title ID com
os inputs do build. O loader imprime build ID, hash do runtime e hash VitaGL
no início de `godot_loader.log`. Imports ausentes interrompem a inicialização
com seus nomes no log.

## 3. Aceite no Vita

Instale **Godot ColorRect Test**, title ID `GDCT40001`. Ele usa os requisitos
de runtime do port anterior, incluindo `kubridge` e compilador de shaders.
É um aplicativo separado do preview `GDOT40001` e dos probes.

Esperado: fundo preto e ciclo a cada três segundos:

1. Retângulo vermelho de 160×80 em (80,120).
2. Retângulo verde de 160×80 em (480,120).
3. Retângulo roxo de 80×80 em (300,320).

Coletar de `ux0:/data/`: `godot_loader.log`, `godot_engine.log`,
`godot_stdout.log`, `godot_stderr.log` e `vitaGL.log`, se gerado. Em crash,
preservar também o dump. Uma foto/vídeo curto confirma o comportamento visual;
os logs `[colorrect] phase=...` comprovam execução de GDScript, não os pixels.

Não declarar PASS apenas por ver o logo ou mensagens de `_process`. O aceite
exige a cena renderizada, posições/tamanhos corretos, transições e ausência de
crash. Este teste ainda não aprova Label, texturas, iluminação ou 3D.

## Diagnóstico da tela preta

No próximo pacote, `godot_stderr.log` também registra `[colorrect] READY`,
as fases e `[vita-loop]` antes/depois dos primeiros três quadros. Marcadores
`[vita-shader]` identificam compilação e link. Copie também
`ux0:/data/godot_shader_vertex.glsl` e `godot_shader_fragment.glsl` para Logs:
eles contêm a última fonte enviada de cada estágio e são sobrescritos a cada
compilação. São evidências de diagnóstico, não confirmação visual.

## Protótipo GXM

O backend nativo mínimo está descrito em [GXM_BACKEND.md](GXM_BACKEND.md).
Após compilar o runtime, acrescente `--backend gxm2d` ao comando de
`tools/package_runtime.py`. Isso inclui o marcador de seleção do backend e
usa o aplicativo separado Godot GXM ColorRect (GDGX40001). O padrão continua
sendo VitaGL. Não há suporte geral a texturas ou shaders nesse primeiro marco.
