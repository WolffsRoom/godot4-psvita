#ifndef VITA_COREDUMP_LWCONDVARINFO_H
#define VITA_COREDUMP_LWCONDVARINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class lwcondvarInfo;

class lwcondvarInfoCondvar {
    friend lwcondvarInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t mutexId();
        std::vector<syncObjectThread>& threads();
        uint32_t workAddress();
        uint32_t mutexWorkAddress();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;
        uint32_t _mutex_id;
        uint32_t _work_address;
        uint32_t _mutex_work_address;
        std::vector<syncObjectThread> _threads;

}; // lwcondvarInfoCondvar


class lwcondvarInfo {
    public:
        lwcondvarInfo(void* buf, uint32_t size);
        std::vector<lwcondvarInfoCondvar>& condvars();

    private:
        std::vector<lwcondvarInfoCondvar> _condvars;
}; // lwcondvarInfo

}; // coredump
}; // vita

#endif