#ifndef VITA_COREDUMP_CONDVARINFO_H
#define VITA_COREDUMP_CONDVARINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class condvarInfo;

class condvarInfoCondvar {
    friend condvarInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t threadId();
        std::vector<syncObjectThread>& threads();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;
        uint32_t _thread_id;
        std::vector<syncObjectThread> _threads;

}; // condvarInfoCondvar


class condvarInfo {
    public:
        condvarInfo(void* buf, uint32_t size);
        std::vector<condvarInfoCondvar>& condvars();

    private:
        std::vector<condvarInfoCondvar> _condvars;
}; // condvarInfo

}; // coredump
}; // vita

#endif