#ifndef VITA_COREDUMP_SEMAPHOREINFO_H
#define VITA_COREDUMP_SEMAPHOREINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class semaphoreInfo;

class semaphoreInfoSemaphore {
    friend semaphoreInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t initialValue();
        uint32_t value();
        uint32_t maxValue();
        std::vector<syncObjectThread>& threads();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;
        uint32_t _initial_value;
        uint32_t _value;
        uint32_t _max_value;
        std::vector<syncObjectThread> _threads;

}; // semaphoreInfoSemaphore


class semaphoreInfo {
    public:
        semaphoreInfo(void* buf, uint32_t size);
        std::vector<semaphoreInfoSemaphore>& semaphores();

    private:
        std::vector<semaphoreInfoSemaphore> _semaphores;
}; // semaphoreInfo

}; // coredump
}; // vita

#endif