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
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/VitaGL.cmake")

set(VITA_APP_NAME "Godot 4 (Vita)" CACHE STRING "Vita application name")
set(VITA_TITLEID  "GDOT40001" CACHE STRING "Vita title ID")
set(VITA_VERSION  "01.00" CACHE STRING "Vita package version")
set(VITA_RUNTIME "" CACHE FILEPATH "Explicit libgodot.so to package")
set(VITA_GAME_DATA "${CMAKE_CURRENT_SOURCE_DIR}/game_data" CACHE PATH "Project to package")
set(VITA_RUNTIME_BUILD_ID "manual" CACHE STRING "Runtime build identifier")
if(NOT VITA_RUNTIME OR NOT EXISTS "${VITA_RUNTIME}")
  message(FATAL_ERROR "Set VITA_RUNTIME to the newly built libgodot.so")
endif()
if(NOT VITA_RUNTIME_BUILD_ID MATCHES "^[A-Za-z0-9_.-]+$")
  message(FATAL_ERROR "Invalid runtime build identifier")
endif()
file(SHA256 "${VITA_RUNTIME}" VITA_RUNTIME_SHA256)

add_compile_options(-Wall -O2 -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard)

add_executable(${PROJECT_NAME}
  main.c
  so_util.c
  default_dynlib.c
  tls_stubs.c
  diag.c
  dl_stubs.c
  # Bridges VitaGL's non-underscore sceAppMgrGetBudgetInfo() (memory-budget
  # query) to the raw NID _sceAppMgrGetBudgetInfo in the SDK stubs.
  sdk_shims.c
  gxm_bridge.c
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  VITA_RUNTIME_BUILD_ID="${VITA_RUNTIME_BUILD_ID}"
  VITA_RUNTIME_SHA256="${VITA_RUNTIME_SHA256}"
  VITA_GL_SHA256="${VITA_GL_SHA256}"
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
  # Explicit archive and matching headers, shared with the regression probe.
  vita_port_gl
  vita2d
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

option(VITA_GXM2D "Select experimental native GXM canvas backend" OFF)
set(GXM_PACKAGE_FILES)
if(VITA_GXM2D)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gxm2d.cfg" "native-canvas-v2\n")
  list(APPEND GXM_PACKAGE_FILES FILE "${CMAKE_CURRENT_BINARY_DIR}/gxm2d.cfg" gxm2d.cfg)
endif()
# Optional game-specific LiveArea and pre-engine loading animation.
# Default packaging deliberately remains unchanged for ColorRect probes.
set(VITA_LIVEAREA_DIR "" CACHE PATH "Optional LiveArea asset folder (icon0, pic0, bg0, startup, template)")
set(VITA_BOOT_ANIMATION "" CACHE FILEPATH "Optional DVAN1 native loading animation data")
set(VITA_BRANDING_FILES)
if(VITA_LIVEAREA_DIR)
  foreach(ASSET IN ITEMS icon0.png pic0.png bg0.png startup.png template.xml)
    if(NOT EXISTS "${VITA_LIVEAREA_DIR}/${ASSET}")
      message(FATAL_ERROR "Missing LiveArea asset: ${VITA_LIVEAREA_DIR}/${ASSET}")
    endif()
  endforeach()
  list(APPEND VITA_BRANDING_FILES
    FILE "${VITA_LIVEAREA_DIR}/icon0.png" "sce_sys/icon0.png"
    FILE "${VITA_LIVEAREA_DIR}/pic0.png" "sce_sys/pic0.png"
    FILE "${VITA_LIVEAREA_DIR}/bg0.png" "sce_sys/livearea/contents/bg0.png"
    FILE "${VITA_LIVEAREA_DIR}/startup.png" "sce_sys/livearea/contents/startup.png"
    FILE "${VITA_LIVEAREA_DIR}/template.xml" "sce_sys/livearea/contents/template.xml")
endif()
if(VITA_BOOT_ANIMATION)
  if(NOT EXISTS "${VITA_BOOT_ANIMATION}")
    message(FATAL_ERROR "Missing native boot animation: ${VITA_BOOT_ANIMATION}")
  endif()
  target_sources(${PROJECT_NAME} PRIVATE boot_splash.c)
  target_compile_definitions(${PROJECT_NAME} PRIVATE VITA_BOOT_ANIMATION_ENABLED=1)
  list(APPEND VITA_BRANDING_FILES FILE "${VITA_BOOT_ANIMATION}" boot_anim.dat)
endif()

option(VITA_FULL_GAME_DATA "Package every resource in the selected Godot game" OFF)
set(VITA_PACKED_GAME_FILES
  FILE ${VITA_GAME_DATA}/project.godot game_data/project.godot
  FILE ${VITA_GAME_DATA}/main.tscn game_data/main.tscn
  FILE ${VITA_GAME_DATA}/main.gd game_data/main.gd
)
if(VITA_FULL_GAME_DATA)
  # Release templates cannot import PNG/font/OGG on the PS Vita.
  file(GLOB_RECURSE VITA_PROJECT_RESOURCES RELATIVE "${VITA_GAME_DATA}" "${VITA_GAME_DATA}/*")
  file(GLOB_RECURSE VITA_IMPORTED_RESOURCES RELATIVE "${VITA_GAME_DATA}"
    "${VITA_GAME_DATA}/.godot/imported/*")
  if(EXISTS "${VITA_GAME_DATA}/.godot/uid_cache.bin")
    list(APPEND VITA_IMPORTED_RESOURCES ".godot/uid_cache.bin")
  endif()
  list(APPEND VITA_PROJECT_RESOURCES ${VITA_IMPORTED_RESOURCES})
  list(REMOVE_DUPLICATES VITA_PROJECT_RESOURCES)
  foreach(VITA_REL_PATH IN LISTS VITA_PROJECT_RESOURCES)
    if(VITA_REL_PATH MATCHES "^\\.godot/editor/" OR VITA_REL_PATH MATCHES "^\\.godot/shader_cache/"
       OR VITA_REL_PATH STREQUAL ".gitignore" OR VITA_REL_PATH STREQUAL ".godot/.gdignore")
      continue()
    endif()
    if(VITA_REL_PATH STREQUAL "main.gd" OR VITA_REL_PATH STREQUAL "main.tscn"
      OR VITA_REL_PATH STREQUAL "project.godot")
      continue()
    endif()
    if(NOT IS_DIRECTORY "${VITA_GAME_DATA}/${VITA_REL_PATH}")
      list(APPEND VITA_PACKED_GAME_FILES
        FILE "${VITA_GAME_DATA}/${VITA_REL_PATH}" "game_data/${VITA_REL_PATH}")
    endif()
  endforeach()
endif()

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  ${GXM_PACKAGE_FILES}
  ${VITA_BRANDING_FILES}
  FILE ${VITA_RUNTIME} libgodot.so
  FILE ${CMAKE_CURRENT_SOURCE_DIR}/../licenses/libvita2d-LICENSE.txt licenses/libvita2d-LICENSE.txt
  FILE ${CMAKE_CURRENT_SOURCE_DIR}/../fonts/Lato-LICENSE.txt licenses/Lato-LICENSE.txt
  # Loose Godot 4.x project (run via --path app0:/game_data).
  ${VITA_PACKED_GAME_FILES}
)
