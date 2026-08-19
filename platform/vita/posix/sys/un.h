/* Vita stub: VitaSDK newlib has no <sys/un.h> (no AF_UNIX domain sockets).
 * Provides just the struct/constant so drivers/unix/net_socket_unix compiles;
 * unix-domain sockets don't work on Vita, which Godot handles at runtime. */
#pragma once

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

#define UNIX_PATH_MAX 108

struct sockaddr_un {
	unsigned short sun_family;
	char sun_path[UNIX_PATH_MAX];
};
