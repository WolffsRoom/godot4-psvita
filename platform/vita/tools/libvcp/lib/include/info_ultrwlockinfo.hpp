#ifndef VITA_COREDUMP_ULTRWLOCKINFO_H
#define VITA_COREDUMP_ULTRWLOCKINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class ultRwlockInfo;

class ultRwlockInfoRwlock {
    friend ultRwlockInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t queuePoolId();
        uint32_t lockStatus();
        uint32_t lockingReaders();
        std::vector<ultSyncThread>& readThreads();
        std::vector<ultSyncThread>& writeThreads();
        uint32_t threadOwnerId();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _queue_pool_id;
        uint32_t _lock_status;
        uint32_t _locking_readers;
        std::vector<ultSyncThread> _read_threads;
        std::vector<ultSyncThread> _write_threads;
        uint32_t _thread_owner_id;

}; // ultRwlockInfoRwlock


class ultRwlockInfo {
    public:
        ultRwlockInfo(void* buf, uint32_t size);
        std::vector<ultRwlockInfoRwlock>& rwlocks();

    private:
        std::vector<ultRwlockInfoRwlock> _rwlocks;
}; // ultRwlockInfo

}; // coredump
}; // vita

#endif