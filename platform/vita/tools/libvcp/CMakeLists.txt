cmake_minimum_required(VERSION 3.12)

# Detect if we are the top level CMakeLists.txt or are we included in some
# other project
if(NOT DEFINED PROJECT_NAME)
  set(IS_TOP_PROJECT TRUE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(vcp VERSION 1.0 LANGUAGES C CXX)

add_definitions(-Wall)

include(GNUInstallDirs)

set(SOURCES
  lib/src/coredumpparser.cpp
  lib/src/info_corefileinfo.cpp
  lib/src/info_hwinfo.cpp
  lib/src/info_buildverinfo.cpp
  lib/src/info_systeminfo.cpp
  lib/src/info_ttyinfo.cpp
  lib/src/info_screenshotinfo.cpp
  lib/src/info_sysdeviceinfo.cpp
  lib/src/info_appinfo.cpp
  lib/src/info_processinfo.cpp
  lib/src/info_deviceinfo.cpp
  lib/src/info_extnlprocessinfo.cpp
  lib/src/info_fileinfo.cpp
  lib/src/info_applistinfo.cpp
  lib/src/info_stackinfo.cpp
  lib/src/info_moduleinfo.cpp
  lib/src/info_memblockinfo.cpp
  lib/src/info_budgetinfo.cpp
  lib/src/info_libraryinfo.cpp
  lib/src/info_eventloginfo.cpp
  lib/src/info_semaphoreinfo.cpp
  lib/src/info_eventflaginfo.cpp
  lib/src/info_mutexinfo.cpp
  lib/src/info_lwmutexinfo.cpp
  lib/src/info_msgpipeinfo.cpp
  lib/src/info_callbackinfo.cpp
  lib/src/info_timerinfo.cpp
  lib/src/info_rwlockinfo.cpp
  lib/src/info_condvarinfo.cpp
  lib/src/info_lwcondvarinfo.cpp
  lib/src/info_simpleeventinfo.cpp
  lib/src/info_ultinfo.cpp
  lib/src/info_ultsemaphoreinfo.cpp
  lib/src/info_ultcondvarinfo.cpp
  lib/src/info_ultmutexinfo.cpp
  lib/src/info_ultrwlockinfo.cpp
  lib/src/info_ultwqpoolinfo.cpp
  lib/src/info_ultqpoolinfo.cpp
  lib/src/info_ultqueueinfo.cpp
  lib/src/info_fiberinfo.cpp
  lib/src/info_threadinfo.cpp
  lib/src/info_threadreginfo.cpp
  lib/src/info_gpuinfo.cpp
)

add_library(vcp STATIC ${SOURCES})
add_library(vcp::vcp ALIAS vcp)

target_include_directories(
    vcp
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/src>
        $<INSTALL_INTERFACE:include/vcp>
)


add_subdirectory(cli)


# If this is the top level project, add in logic to install vcp
if (IS_TOP_PROJECT)
    include(CMakePackageConfigHelpers)

    # Create a file that includes the current project version. This will be
    # installed with the vcp CMake package.
    write_basic_package_version_file(
        "${PROJECT_NAME}ConfigVersion.cmake"
        VERSION
            ${PROJECT_VERSION}
        COMPATIBILITY
            SameMajorVersion)
    
    # Create the default ${PROJECT_NAME}Config.cmake file which will be
    # installed and found by calls to `find_package(vcp)`.
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION
            ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    # Install the previously generated "config" and "version" files
    install(
        FILES
            "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION
            ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(
        DIRECTORY
            lib/include/
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}/vcp)

    # Create a ${PROJECT_NAME}Targets.cmake file that is referenced by the
    # ${PROJECT_NAME}Config.cmake file and includes the target information
    # needed to compile/link against all targets exported under the
    # ${PROJECT_NAME}_Targets export
    install(
        EXPORT
            ${PROJECT_NAME}_Targets
        FILE
            ${PROJECT_NAME}Targets.cmake
        NAMESPACE
            ${PROJECT_NAME}::
        DESTINATION
            ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    # Add the vcp target to the ${PROJECT_NAME}_Targets export
    install(
        TARGETS
            vcp
        EXPORT
            ${PROJECT_NAME}_Targets)
endif()
