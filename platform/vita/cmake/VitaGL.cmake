# Use the same explicit VitaGL archive and headers for probes and the loader.
# Never silently fall back to an unrelated SDK installation.
set(VITA_GL_ROOT "" CACHE PATH "Patched VitaGL source/build directory")
set(VITA_STUB_DIR "" CACHE PATH "Optional directory of regenerated SDK stubs")
if(NOT VITA_GL_ROOT)
  message(FATAL_ERROR "Set VITA_GL_ROOT to the patched VitaGL source/build directory.")
endif()
get_filename_component(VITA_GL_ROOT "${VITA_GL_ROOT}" ABSOLUTE)
set(VITA_GL_LIBRARY "${VITA_GL_ROOT}/libvitaGL.a")
if(NOT EXISTS "${VITA_GL_LIBRARY}" OR NOT EXISTS "${VITA_GL_ROOT}/source/vitaGL.h")
  message(FATAL_ERROR "VITA_GL_ROOT must contain libvitaGL.a and source/vitaGL.h")
endif()
file(SHA256 "${VITA_GL_LIBRARY}" VITA_GL_SHA256)
message(STATUS "VitaGL: ${VITA_GL_LIBRARY} (SHA256 ${VITA_GL_SHA256})")
add_library(vita_port_gl STATIC IMPORTED)
set_target_properties(vita_port_gl PROPERTIES
  IMPORTED_LOCATION "${VITA_GL_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${VITA_GL_ROOT}/source"
)
if(VITA_STUB_DIR)
  if(NOT IS_DIRECTORY "${VITA_STUB_DIR}")
    message(FATAL_ERROR "VITA_STUB_DIR does not exist: ${VITA_STUB_DIR}")
  endif()
  link_directories(BEFORE "${VITA_STUB_DIR}")
endif()
