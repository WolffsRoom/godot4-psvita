#ifndef VITA_COREDUMP_ULTWQPOOLINFO_H
#define VITA_COREDUMP_ULTWQPOOLINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class ultWqpoolInfo;

class ultWqpoolInfoPool {
    friend ultWqpoolInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t maxWaiters();
        uint32_t currentWaiters();
        uint32_t maxSyncObjs();
        uint32_t currentSyncObjs();
    private:
        uint32_t _id;
        std::string _name;

        uint32_t _max_waiters;
        uint32_t _current_waiters;
        uint32_t _max_sync_objs;
        uint32_t _current_sync_objs;
}; // ultWqpoolInfoPool


class ultWqpoolInfo {
    public:
        ultWqpoolInfo(void* buf, uint32_t size);
        std::vector<ultWqpoolInfoPool>& pools();

    private:
        std::vector<ultWqpoolInfoPool> _pools;
}; // ultWqpoolInfo

}; // coredump
}; // vita

#endif