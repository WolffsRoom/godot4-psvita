"""
Vita (PlayStation Vita) platform detection / configuration for Godot 4.x.

Adapted from the Godot 3.x Vita port (Stary2001 / EIREXE / PVR_PSP2) to the
Godot 4.x build system conventions:
  * `target` values are now: editor | template_debug | template_release
  * `arch` replaces `bits`
  * Vulkan is the default RenderingDevice; here it is force-disabled.
  * Only the OpenGL "Compatibility" renderer (drivers/gles3) is enabled, backed
    by VitaGL. See platform/vita/README.md for the ES3-vs-VitaGL caveat.

This file is intentionally VitaGL-only (no PVR_PSP2 path) to keep the initial
4.x bring-up small. PVR_PSP2 never offered GLES3 either, so it does not help the
Compatibility renderer.
"""

import os
import sys
from typing import TYPE_CHECKING

from methods import print_error, print_warning

if TYPE_CHECKING:
    from SCons.Script.SConscript import SConsEnvironment


def get_name():
    return "Vita"


def can_build():
    if os.name != "posix" and sys.platform != "win32":
        # VitaSDK runs under MSYS2/WSL on Windows too; we only warn.
        pass
    vitasdk = os.environ.get("VITASDK")
    if vitasdk is None or not os.path.exists(vitasdk):
        # Silent by default: printing here would spam every `scons --help`.
        return False
    return True


def get_opts():
    from SCons.Variables import BoolVariable

    return [
        BoolVariable("use_vitagl", "Use VitaGL as the GL backend (required for the Compatibility renderer)", True),
        BoolVariable("vita_shared", "Build libgodot.so for the Vita loader", True),
        BoolVariable("vita_gxm3d", "Build a separate experimental GXM 3D runtime", False),
        ("vita_gl_root", "Explicit patched VitaGL directory (library and source/vitaGL.h)", ""),
        BoolVariable("touch", "Enable touch input", True),
        BoolVariable("debug_symbols", "Add debugging symbols to release/template builds", False),
    ]


def get_doc_classes():
    return []


def get_doc_path():
    return "doc_classes"


def get_flags():
    # Force the engine into the only viable Vita configuration.
    return {
        "arch": "arm32",
        "target": "template_release",
        # Renderer selection: OpenGL Compatibility only, no Vulkan RenderingDevice.
        "vulkan": False,
        "d3d12": False,
        "metal": False,
        "opengl3": True,
        # No editor / no export templates tooling on-device.
        "tools": False,
        # Modules that pull heavy or unavailable deps on Vita.
        "module_mono_enabled": False,
        "module_openxr_enabled": False,
        "module_webrtc_enabled": False,
        "module_websocket_enabled": False,
        "module_multiplayer_enabled": True,
        # Thirdparty modules that don't support Vita / pull unavailable POSIX
        # headers. Disabled for the initial bring-up.
        "module_jolt_physics_enabled": False,  # Jolt: "#error Unknown platform"
        "module_csg_enabled": False,  # manifold svd.h fails to build
        "module_upnp_enabled": False,  # miniupnpc needs net/if.h, sys/un.h
        "module_mbedtls_enabled": False,  # no mbedtls_ms_time on Vita newlib
        "builtin_pcre2": False,
        "builtin_zlib": False,
        "builtin_libpng": False,
        "builtin_freetype": False,
        "builtin_libogg": False,
        "builtin_libvorbis": False,
        "builtin_libtheora": False,
        "builtin_opus": False,
        "builtin_zstd": False,
        # Not in VitaSDK / needs builtin.
        "builtin_enet": True,
        "builtin_mbedtls": True,
    }


def configure(env: "SConsEnvironment"):
    # Validate arch (Vita is armv7 hard-float NEON only).
    supported_arches = ["arm32"]
    if env["arch"] not in supported_arches:
        print_error(
            'Unsupported CPU architecture "%s" for Vita. Only "arm32" is supported.' % env["arch"]
        )
        sys.exit(255)

    ## Toolchain (VitaSDK)
    vitasdk = os.environ["VITASDK"]
    tool_prefix = os.path.join(vitasdk, "bin", "arm-vita-eabi-")

    env["CC"] = tool_prefix + "gcc"
    env["CXX"] = tool_prefix + "g++"
    env["LD"] = tool_prefix + "ld"
    env["AR"] = tool_prefix + "ar"
    env["RANLIB"] = tool_prefix + "ranlib"
    env["STRIP"] = tool_prefix + "strip"
    env["OBJCOPY"] = tool_prefix + "objcopy"
    env["VITASDK_BIN"] = os.path.join(vitasdk, "bin") + os.sep

    # pkg-config for VitaSDK-provided libraries.
    pkgconfig = os.path.join(vitasdk, "arm-vita-eabi", "lib", "pkgconfig")
    os.environ["PKG_CONFIG_PATH"] = pkgconfig
    env["ENV"]["PKG_CONFIG_PATH"] = pkgconfig

    env.Prepend(
        CPPPATH=[
            os.path.join(vitasdk, "arm-vita-eabi", "include"),
            os.path.join(vitasdk, "arm-vita-eabi", "include", "freetype2"),
        ]
    )
    env.Append(LIBPATH=[os.path.join(vitasdk, "arm-vita-eabi", "lib")])

    vita_gl_root = os.path.abspath(env["vita_gl_root"])
    if not env["vita_gl_root"] or not os.path.isfile(os.path.join(vita_gl_root, "source", "vitaGL.h")):
        print_error("Set vita_gl_root to the identified patched VitaGL build directory.")
        sys.exit(255)
    if not os.path.isfile(os.path.join(vita_gl_root, "libvitaGL.a")):
        print_error("vita_gl_root must contain libvitaGL.a.")
        sys.exit(255)
    env.Prepend(CPPPATH=[os.path.join(vita_gl_root, "source")])
    env.Prepend(LIBPATH=[vita_gl_root])

    ## Build type (4.x target names)
    if env["target"] == "template_release":
        if env["optimize"] == "speed" or env["optimize"] == "speed_trace":
            env.Append(CCFLAGS=["-O3", "-ffast-math"])
        else:  # size (recommended on Vita, 512 MB RAM budget)
            env.Append(CCFLAGS=["-Os"])
    elif env["target"] == "template_debug":
        env.Append(CCFLAGS=["-Og", "-g"])
        env.Append(CPPDEFINES=["DEBUG_ENABLED"])
    else:
        print_error('The "editor" target is not supported on Vita; use template_debug/template_release.')
        sys.exit(255)

    if env["debug_symbols"]:
        env.Append(CCFLAGS=["-g2"])

    ## Architecture
    env["arch"] = "arm32"

    ## CPU / FPU flags for the SGX-era ARM Cortex-A9 + NEON.
    env.Append(
        CCFLAGS=[
            "-mcpu=cortex-a9",
            "-mfpu=neon",
            "-mfloat-abi=hard",
            "-ftree-vectorize",
            "-fno-optimize-sibling-calls",
            "-Wno-attributes",
            "-fpermissive",
            # so-loader model: the engine is built as a PIC shared object
            # (libgodot.so) loaded at runtime, which sidesteps the vita-elf-create
            # `-q`/--emit-relocs "invalid operation" that a 31 MB executable hits
            # (Thumb branch-range veneers + emit-relocs). See platform/vita/README.md.
            "-fPIC",
        ]
    )

    ## Preprocessor defines that the Godot core + our platform code key off.
    env.Append(
        CPPDEFINES=[
            "VITA_ENABLED",
            "UNIX_ENABLED",  # reuse drivers/unix (OS_Unix base, file/dir/net POSIX backends)
            "__psp2__",
            "PTHREAD_ENABLED",
            "PTHREAD_NO_RENAME",
            "LIBC_FILEIO_ENABLED",
            "_POSIX_TIMERS",
            # GL Compatibility renderer.
            "GLES3_ENABLED",
            "GLES_ENABLED",
            "GL_GLEXT_PROTOTYPES",
            # NOTE: no EGL_ENABLED — VitaGL owns the GXM context directly, so we
            # avoid drivers/egl and the platform_egl.h requirement.
        ]
    )

    # Minimal stub POSIX headers VitaSDK newlib lacks (xattr/ioctl/un/ifaddrs/
    # syslog), consumed by drivers/unix. Must precede the SDK include dir.
    env.Prepend(CPPPATH=["#platform/vita/posix"])

    if env["use_vitagl"]:
        env.Append(CPPDEFINES=["VITAGL"])

    if env["touch"]:
        env.Append(CPPDEFINES=["TOUCH_ENABLED"])

    env.Append(CPPPATH=["#platform/vita"])

    # Several VitaSDK newlib headers (netdb.h, ...) use the C99 `restrict`
    # keyword, which C++ rejects. Map it to GCC's C++ __restrict extension.
    env.Append(CXXFLAGS=["-Drestrict=__restrict"])

    ## Linker: -q keeps relocations so vita-elf-create can build the VELF.
    ## --no-relax: linker relaxation + --emit-relocs (-q) can produce relocations
    ## the emitter chokes on ("final link failed: invalid operation") on large
    ## binaries; disabling relaxation avoids that.
    env.Append(LINKFLAGS=["-Wl,-q", "-Wl,--no-relax"])

    ## Libraries.
    # VitaGL + its GXM / shader compiler dependencies, then the media/codec
    # libraries Godot's builtins were disabled in favour of.
    common_libs = [
        "jpeg",
        "png",
        "freetype",
        "webp",
        "opus",
        "vorbis",
        "ogg",
        "theora",
        "z",
        "zstd",
        "bz2",  # freetype (ftbzip2) + minizip reference bzip2.
        "pcre2-32",
        "pthread",
    ]

    if env["use_vitagl"]:
        gl_libs = [
            "vitaGL",
            "vitashark",
            "SceShaccCgExt",
            "SceShaccCg_stub",
            "SceGxm_stub",
            "SceKernelDmacMgr_stub",
            "mathneon",
        ]
    else:
        # PVR_PSP2 EGL/GLESv2 path (kept for reference; not GLES3-capable).
        gl_libs = [
            "libgpu_es4_ext_stub_weak",
            "libIMGEGL_stub_weak",
            "libGLESv2_stub_weak",
        ]

    sce_stubs = [
        "SceAppMgr_stub",
        "SceAppUtil_stub",
        "SceAudio_stub",
        "SceCommonDialog_stub",
        "SceCtrl_stub",
        "SceDisplay_stub",
        "SceFios2_stub",
        "SceIme_stub",
        "SceIofilemgr_stub",
        "SceKernelThreadMgr_stub",
        "SceLibKernel_stub",
        "SceMotion_stub",
        "SceNetCtl_stub",
        "SceNet_stub",
        "ScePower_stub",
        "SceSblSsMgr_stub",
        "SceSysmodule_stub",
        "SceTouch_stub",
        "taihen_stub",
    ]

    env.Append(LIBS=["dl"] + gl_libs + common_libs + sce_stubs)

    ## Godot uses SharedObject on some platforms; Vita links a static executable.
    env["SHOBJSUFFIX"] = ".os"
    env["OBJSUFFIX"] = ".o"

    if env["use_vitagl"]:
        print_warning(
            "Vita: building with VitaGL. The Compatibility renderer needs OpenGL ES 3.0; "
            "VitaGL is ~GLES2. Expect unresolved ES3 symbols at link time — see platform/vita/README.md."
        )
