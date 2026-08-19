#ifndef VITA_COREDUMP_MUTEXINFO_H
#define VITA_COREDUMP_MUTEXINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class mutexInfo;

class mutexInfoMutex {
    friend mutexInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t initialValue();
        uint32_t value();
        uint32_t threadOwnerId();
        uint32_t ceilingPriority();
        std::vector<syncObjectThread>& threads();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;
        uint32_t _initial_value;
        uint32_t _value;
        uint32_t _thread_owner_id;
        uint32_t _ceiling_priority;

        std::vector<syncObjectThread> _threads;

}; // mutexInfoMutex


class mutexInfo {
    public:
        mutexInfo(void* buf, uint32_t size);
        std::vector<mutexInfoMutex>& mutexes();

    private:
        std::vector<mutexInfoMutex> _mutexes;
}; // mutexInfo

}; // coredump
}; // vita

#endif