/* Vita stub: VitaSDK newlib has no <net/if.h>. Interface enumeration reports
 * nothing (paired with the ifaddrs.h stub), so drivers/unix/ip_unix compiles
 * and simply finds no named interfaces. */
#pragma once

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

static inline unsigned int if_nametoindex(const char *) { return 0; }
static inline char *if_indextoname(unsigned int, char *ifname) { return ifname; }
