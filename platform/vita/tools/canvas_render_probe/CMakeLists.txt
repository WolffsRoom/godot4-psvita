cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Define the VITASDK environment variable to point to the VitaSDK root.")
  endif()
endif()

project(canvas_render_probe C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Canvas Render Probe")
set(VITA_TITLEID  "GDCR30001")
set(VITA_VERSION  "01.00")

add_compile_options(-Wall -O2 -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard)

add_executable(${PROJECT_NAME} main.c sdk_shims.c)

target_link_libraries(${PROJECT_NAME}
  vitaGL
  vitashark
  SceShaccCgExt
  taihen_stub
  SceShaccCg_stub
  mathneon
  SceGxm_stub
  SceKernelDmacMgr_stub
  SceCommonDialog_stub
  SceDisplay_stub
  SceAppMgr_stub
  SceSysmodule_stub
  ScePower_stub
  SceLibKernel_stub
  m
  stdc++
  supc++
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
