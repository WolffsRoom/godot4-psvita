/* Vita stub: VitaSDK newlib has no <ifaddrs.h>. getifaddrs() reports no
 * interfaces (returns -1, *ifap = NULL) so drivers/unix/ip_unix's enumeration
 * loop simply finds nothing. Struct fields exist only so the loop body compiles. */
#pragma once

#include <sys/socket.h>

struct ifaddrs {
	struct ifaddrs *ifa_next;
	char *ifa_name;
	unsigned int ifa_flags;
	struct sockaddr *ifa_addr;
	struct sockaddr *ifa_netmask;
	struct sockaddr *ifa_dstaddr;
	void *ifa_data;
};

static inline int getifaddrs(struct ifaddrs **ifap) {
	if (ifap) {
		*ifap = 0;
	}
	return -1;
}
static inline void freeifaddrs(struct ifaddrs *) {}
