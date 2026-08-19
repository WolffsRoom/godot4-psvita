#ifndef VITA_COREDUMP_LWMUTEXINFO_H
#define VITA_COREDUMP_LWMUTEXINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class lwmutexInfo;

class lwmutexInfoMutex {
    friend lwmutexInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t workAddress();
        uint32_t initialValue();
        uint32_t value();
        uint32_t threadOwnerId();
        std::vector<syncObjectThread>& threads();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _work_address;
        uint32_t _initial_value;
        uint32_t _value;
        uint32_t _thread_owner_id;

        std::vector<syncObjectThread> _threads;

}; // lwmutexInfoMutex


class lwmutexInfo {
    public:
        lwmutexInfo(void* buf, uint32_t size);
        std::vector<lwmutexInfoMutex>& mutexes();

    private:
        std::vector<lwmutexInfoMutex> _mutexes;
}; // lwmutexInfo

}; // coredump
}; // vita

#endif