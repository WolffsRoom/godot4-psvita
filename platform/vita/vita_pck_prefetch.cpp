/**************************************************************************/
/*  vita_pck_prefetch.cpp                                                 */
/**************************************************************************/

#include "vita_pck_prefetch.h"

#include "core/os/mutex.h"
#include "core/os/semaphore.h"
#include "core/os/thread.h"

#include <psp2/io/fcntl.h>
#include <psp2/kernel/cpu.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace {

constexpr uint64_t PREFETCH_CAPACITY = 3 * 128 * 1024;

// GODOT_VITA_PCK_PREFETCH_PAUSE=1 (OS.set_environment from a script) makes
// the next reads bypass the read-ahead window while the worker stays alive,
// so one run can compare reads with and without the prefetch. Each change is
// logged with the counters so far. Only meant for test projects: set it
// between reads, from the main thread.
bool prefetch_paused() {
	const char *value = getenv("GODOT_VITA_PCK_PREFETCH_PAUSE");
	return value && value[0] == '1';
}

class VitaPCKPrefetch {
	Thread worker;
	Mutex mutex;
	Semaphore wake;
	bool started = false;
	bool exit_requested = false;
	bool request_pending = false;
	uint64_t request_serial = 0;
	String request_pack;
	uint64_t request_offset = 0;
	uint64_t request_length = 0;

	SceUID buffer_block = -1;
	uint8_t *buffer = nullptr;
	String cache_pack;
	uint64_t cache_offset = 0;
	uint64_t cache_length = 0;
	uint64_t cache_consumed = 0;

	uint64_t requests = 0;
	uint64_t hits = 0;
	uint64_t hit_bytes = 0;
	uint64_t disk_bytes = 0;
	uint64_t unused_bytes = 0;
	bool paused = false;

	bool _check_paused() {
		bool now = prefetch_paused();
		if (now != paused) {
			MutexLock lock(mutex);
			paused = now;
			fprintf(stderr, "[vita-io] PCK prefetch paused=%d requests=%llu hits=%llu hit_bytes=%llu disk_bytes=%llu\n", paused ? 1 : 0,
					(unsigned long long)requests, (unsigned long long)hits, (unsigned long long)hit_bytes, (unsigned long long)disk_bytes);
			fflush(stderr);
		}
		return now;
	}

	static void _thread_entry(void *p_userdata) {
		static_cast<VitaPCKPrefetch *>(p_userdata)->_thread_loop();
	}

	void _thread_loop() {
		int affinity = sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_SYSTEM);
		fprintf(stderr, "[vita-io] PCK prefetch worker cpu3_affinity_rc=0x%08x capacity=%u\n", (unsigned int)affinity, (unsigned int)PREFETCH_CAPACITY);
		fflush(stderr);
		while (true) {
			wake.wait();
			String pack;
			uint64_t offset = 0;
			uint64_t length = 0;
			uint64_t serial = 0;
			{
				MutexLock lock(mutex);
				if (exit_requested) {
					break;
				}
				if (!request_pending) {
					continue;
				}
				pack = request_pack;
				offset = request_offset;
				length = request_length;
				serial = request_serial;
				request_pending = false;
				// The worker overwrites the single staging buffer outside the lock.
				// Invalidate the published window first so the game thread can never
				// copy bytes while DMA/storage is replacing them.
				unused_bytes += cache_length > cache_consumed ? cache_length - cache_consumed : 0;
				cache_pack = String();
				cache_length = 0;
				cache_consumed = 0;
			}
			CharString path = pack.utf8();
			SceUID fd = sceIoOpen(path.get_data(), SCE_O_RDONLY, 0);
			int64_t read = -1;
			if (fd >= 0 && sceIoLseek(fd, offset, SCE_SEEK_SET) >= 0) {
				read = sceIoRead(fd, buffer, length);
			}
			if (fd >= 0) {
				sceIoClose(fd);
			}
			MutexLock lock(mutex);
			if (read > 0 && serial == request_serial && !request_pending) {
				cache_pack = pack;
				cache_offset = offset;
				cache_length = read;
				cache_consumed = 0;
				disk_bytes += read;
			}
		}
	}

public:
	bool ensure_started() {
		MutexLock lock(mutex);
		if (started) {
			return true;
		}
		const char *capunlocker = getenv("GODOT_VITA_CAPUNLOCKER");
		const char *prefetch = getenv("GODOT_VITA_PCK_PREFETCH");
		if (!capunlocker || capunlocker[0] != '1' || (prefetch && prefetch[0] == '0')) {
			return false;
		}
		buffer_block = sceKernelAllocMemBlock("godot_pck_prefetch", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, PREFETCH_CAPACITY, nullptr);
		if (buffer_block < 0 || sceKernelGetMemBlockBase(buffer_block, reinterpret_cast<void **>(&buffer)) < 0 || !buffer) {
			if (buffer_block >= 0) {
				sceKernelFreeMemBlock(buffer_block);
			}
			buffer_block = -1;
			buffer = nullptr;
			return false;
		}
		Thread::Settings settings;
		settings.priority = Thread::PRIORITY_NORMAL;
		worker.start(_thread_entry, this, settings);
		started = worker.is_started();
		if (!started) {
			sceKernelFreeMemBlock(buffer_block);
			buffer_block = -1;
			buffer = nullptr;
		}
		return started;
	}

	bool try_read(const String &p_pack, uint64_t p_offset, uint8_t *p_dst, uint64_t p_length) {
		if (!ensure_started() || p_length == 0 || _check_paused()) {
			return false;
		}
		MutexLock lock(mutex);
		if (cache_pack != p_pack || p_offset < cache_offset || p_offset + p_length > cache_offset + cache_length) {
			return false;
		}
		uint64_t relative = p_offset - cache_offset;
		memcpy(p_dst, buffer + relative, p_length);
		cache_consumed = MAX(cache_consumed, relative + p_length);
		hits++;
		hit_bytes += p_length;
		return true;
	}

	void schedule(const String &p_pack, uint64_t p_offset, uint64_t p_length) {
		if (!ensure_started() || p_length == 0 || _check_paused()) {
			return;
		}
		bool post = false;
		{
			MutexLock lock(mutex);
			requests++;
			if (cache_pack == p_pack && p_offset >= cache_offset && p_offset < cache_offset + cache_length) {
				return;
			}
			if (request_pending && request_pack == p_pack && request_offset == p_offset) {
				return;
			}
			request_pack = p_pack;
			request_offset = p_offset;
			request_length = MIN(p_length, PREFETCH_CAPACITY);
			request_serial++;
			request_pending = true;
			post = true;
		}
		if (post) {
			wake.post();
		}
	}

	void shutdown() {
		{
			MutexLock lock(mutex);
			if (!started) {
				return;
			}
			exit_requested = true;
		}
		wake.post();
		worker.wait_to_finish();
		{
			MutexLock lock(mutex);
			unused_bytes += cache_length > cache_consumed ? cache_length - cache_consumed : 0;
			fprintf(stderr, "[vita-io] PCK prefetch requests=%llu hits=%llu hit_bytes=%llu disk_bytes=%llu unused_bytes=%llu\n",
					(unsigned long long)requests, (unsigned long long)hits, (unsigned long long)hit_bytes,
					(unsigned long long)disk_bytes, (unsigned long long)unused_bytes);
			fflush(stderr);
			started = false;
		}
		if (buffer_block >= 0) {
			sceKernelFreeMemBlock(buffer_block);
			buffer_block = -1;
			buffer = nullptr;
		}
	}
};

VitaPCKPrefetch *singleton = nullptr;

VitaPCKPrefetch *get_singleton() {
	if (!singleton) {
		singleton = memnew(VitaPCKPrefetch);
	}
	return singleton;
}

} // namespace

bool vita_pck_prefetch_is_enabled() {
	return get_singleton()->ensure_started();
}

bool vita_pck_prefetch_try_read(const String &p_pack, uint64_t p_offset, uint8_t *p_dst, uint64_t p_length) {
	return get_singleton()->try_read(p_pack, p_offset, p_dst, p_length);
}

void vita_pck_prefetch_schedule(const String &p_pack, uint64_t p_offset, uint64_t p_length) {
	get_singleton()->schedule(p_pack, p_offset, p_length);
}

void vita_pck_prefetch_shutdown() {
	if (singleton) {
		singleton->shutdown();
		memdelete(singleton);
		singleton = nullptr;
	}
}

