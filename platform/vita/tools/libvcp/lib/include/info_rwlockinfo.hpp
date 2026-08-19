#ifndef VITA_COREDUMP_RWLOCKINFO_H
#define VITA_COREDUMP_RWLOCKINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class rwlockInfo;

class rwlockInfoRwlock {
    friend rwlockInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        std::vector<syncObjectThread>& readThreads();
        std::vector<syncObjectThread>& writeThreads();
        uint32_t writeOwnerId();
        uint32_t lockCount();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;

        std::vector<syncObjectThread> _read_threads;
        std::vector<syncObjectThread> _write_threads;
        uint32_t _write_owner_id;
        uint32_t _lock_count;

}; // rwlockInfoRwlock


class rwlockInfo {
    public:
        rwlockInfo(void* buf, uint32_t size);
        std::vector<rwlockInfoRwlock>& rwlocks();

    private:
        std::vector<rwlockInfoRwlock> _rwlocks;
}; // rwlockInfo

}; // coredump
}; // vita

#endif