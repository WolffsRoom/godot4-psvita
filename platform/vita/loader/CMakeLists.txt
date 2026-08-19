cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Define the VITASDK environment variable.")
  endif()
endif()

project(godot_loader C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Godot 4 (Vita)")
set(VITA_TITLEID  "GDOT40001")
set(VITA_VERSION  "01.00")

add_compile_options(-Wall -O2 -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard)

# Local no-splash VitaGL + regenerated stubs (SceAppMgr with SharedFb/budget)
# must precede the SDK libs. NOTE: link_directories() only affects targets
# created AFTER it, so it MUST come before add_executable() — otherwise no -L
# reaches the link line and -lvitaGL silently resolves to the SDK's VitaGL
# (which lacks our FILE_LOG/SHARK_LOG diagnostics). See loader/BUILDING notes.
link_directories(/tmp/stubs /tmp/vgl)

add_executable(${PROJECT_NAME}
  main.c
  so_util.c
  default_dynlib.c
  tls_stubs.c
  diag.c
  dl_stubs.c
  # Bridges VitaGL's non-underscore sceAppMgrGetBudgetInfo() (memory-budget
  # query) to the raw NID _sceAppMgrGetBudgetInfo the regenerated /tmp/stubs
  # exports. Needed because -L/tmp/stubs shadows the SDK's fuller SceAppMgr stub.
  sdk_shims.c
)

target_link_libraries(${PROJECT_NAME}
  # --start-group/--end-group: the many static libs have circular / order-
  # sensitive deps (vorbis->ogg, SceShaccCgExt->taihen, ...); the group makes
  # the single-pass linker re-scan until resolved.
  -Wl,--start-group
  # so-loader executable memory
  kubridge_stub
  taihen_stub
  # NB: no -ldl — dl_stubs.c provides null-safe dlopen/dlsym (VitaSDK's crashes).
  # GL backend the engine imports (gl*/vgl*). Absolute path (not -lvitaGL) so
  # the linker can NEVER fall back to the SDK's prebuilt VitaGL — we need OUR
  # build with FILE_LOG + SHARK_LOG + DEBUG_GLSL_TRANSLATOR instrumentation.
  /tmp/vgl/libvitaGL.a
  vitashark
  SceShaccCgExt
  SceShaccCg_stub
  mathneon
  # media / codec libs the engine imports
  freetype
  png
  jpeg
  webp
  z
  zstd
  bz2
  pcre2-32
  ogg
  vorbis
  vorbisfile
  theora
  theoradec
  opus
  # C++ runtime + math
  stdc++
  supc++
  m
  # Sce system modules
  SceGxm_stub
  SceKernelDmacMgr_stub
  SceDisplay_stub
  SceCtrl_stub
  SceTouch_stub
  SceMotion_stub
  SceAudio_stub
  SceAppMgr_stub
  SceAppUtil_stub
  SceCommonDialog_stub
  SceSysmodule_stub
  ScePower_stub
  SceRtc_stub
  SceIme_stub
  SceHid_stub
  SceNet_stub
  SceNetCtl_stub
  SceSsl_stub
  SceSblSsMgr_stub
  SceLibKernel_stub
  SceKernelThreadMgr_stub
  SceIofilemgr_stub
  SceFios2_stub
  pthread
  -Wl,--end-group
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_CURRENT_SOURCE_DIR}/libgodot.so libgodot.so
  # Minimal loose Godot 4.x project (run via --path app0:/game_data).
  FILE ${CMAKE_CURRENT_SOURCE_DIR}/game_data/project.godot game_data/project.godot
  FILE ${CMAKE_CURRENT_SOURCE_DIR}/game_data/main.tscn game_data/main.tscn
  FILE ${CMAKE_CURRENT_SOURCE_DIR}/game_data/main.gd game_data/main.gd
)
