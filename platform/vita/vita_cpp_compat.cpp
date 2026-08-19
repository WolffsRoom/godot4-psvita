/**************************************************************************/
/*  vita_cpp_compat.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* POSIX functions VitaSDK declares WITHOUT extern "C", so the C++ caller
 * (drivers/unix/os_unix.cpp) references the C++-mangled name — meaning the
 * definition must also be C++-mangled (i.e. compiled here as C++, not in the
 * C compat unit). Only getrlimit is affected so far. */

struct rlimit;

// Report success with untouched limits so callers proceed (Vita has no rlimits).
int getrlimit(int, struct rlimit *) {
	return 0;
}
