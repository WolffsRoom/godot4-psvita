#ifndef VITA_COREDUMP_ULTMUTEXINFO_H
#define VITA_COREDUMP_ULTMUTEXINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class ultMutexInfo;

class ultMutexInfoMutex {
    friend ultMutexInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t queuePoolId();
        uint32_t lockStatus();
        uint32_t recursiveLockCount();
        uint32_t threadOwnerId();
        std::vector<ultSyncThread>& threads();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _queue_pool_id;
        uint32_t _lock_status;
        uint32_t _recursive_lock_count;
        uint32_t _thread_owner_id;

        std::vector<ultSyncThread> _threads;

}; // ultMutexInfoMutex


class ultMutexInfo {
    public:
        ultMutexInfo(void* buf, uint32_t size);
        std::vector<ultMutexInfoMutex>& mutexes();

    private:
        std::vector<ultMutexInfoMutex> _mutexes;
}; // ultMutexInfo

}; // coredump
}; // vita

#endif