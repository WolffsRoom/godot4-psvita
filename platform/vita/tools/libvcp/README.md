# libvcp
libvcp is a PSVita coredump parser library.

## library
libvcp provides methods to get specific coredump sections as classes with field access methods.  
Note that, however, coredumps are gzipped, but library itself supports parsing only ungzipped files.

## console utilites
Right now there's only one, that dumps parsed coredump as json to stdout.  
It supports both gzipped (original), and ungzipped files.

## sections support
* GPU_INFO: supported only partially, gpu crashdumps only.
* KERNEL_INFO: unsupported (format unknown).
* META_DATA_INFO: unsupported (it's huge and only some parts of format is known).
* USER_INFO: unsupported, data supplied by user, so format can be anything.
* SYSTEM_INFO2: unsupported, contains raw CAF dump.
* GPU_ACT_INFO: unsupported. This section contains info only when a rare and specific gpu bug occurs on devkit in dev-mode.

Everything else is supported.