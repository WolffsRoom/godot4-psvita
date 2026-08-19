#ifndef VITA_COREDUMP_ULTSEMAPHOREINFO_H
#define VITA_COREDUMP_ULTSEMAPHOREINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class ultSemaphoreInfo;

class ultSemaphoreInfoSemaphore {
    friend ultSemaphoreInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t queuePoolId();
        uint32_t currentResources();
        std::vector<ultSyncThread>& threads();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _queue_pool_id;
        uint32_t _current_resources;
        std::vector<ultSyncThread> _threads;

}; // ultSemaphoreInfoSemaphore


class ultSemaphoreInfo {
    public:
        ultSemaphoreInfo(void* buf, uint32_t size);
        std::vector<ultSemaphoreInfoSemaphore>& semaphores();

    private:
        std::vector<ultSemaphoreInfoSemaphore> _semaphores;
}; // ultSemaphoreInfo

}; // coredump
}; // vita

#endif