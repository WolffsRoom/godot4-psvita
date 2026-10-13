cmake_minimum_required(VERSION 3.13)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Set VITASDK")
  endif()
endif()
project(canvas_state_probe C CXX)
include("${VITASDK}/share/vita.cmake" REQUIRED)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/VitaGL.cmake")
set(VITA_PROBE_BUILD_ID "manual" CACHE STRING "Probe build identifier")
if(NOT VITA_PROBE_BUILD_ID MATCHES "^[A-Za-z0-9_.-]+$")
  message(FATAL_ERROR "Invalid build identifier")
endif()
add_executable(canvas_state_probe main.cpp ../ubo_binding_probe/sdk_shims.c)
target_compile_features(canvas_state_probe PRIVATE cxx_std_17)
target_compile_options(canvas_state_probe PRIVATE -Wall -Wextra -O2 -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard)
target_compile_definitions(canvas_state_probe PRIVATE
  VITA_PROBE_BUILD_ID="${VITA_PROBE_BUILD_ID}" VITA_GL_SHA256="${VITA_GL_SHA256}")
target_link_libraries(canvas_state_probe vita_port_gl vitashark SceShaccCgExt
  taihen_stub SceShaccCg_stub mathneon SceGxm_stub SceKernelDmacMgr_stub
  SceCommonDialog_stub SceDisplay_stub SceAppMgr_stub SceSysmodule_stub
  ScePower_stub SceLibKernel_stub m stdc++ supc++)
vita_create_self(canvas_state_probe.self canvas_state_probe)
vita_create_vpk(canvas_state_probe.vpk GDCS40001 canvas_state_probe.self
  VERSION "01.00" NAME "Canvas State Probe")
