#ifndef VITA_COREDUMP_ULTINFO_H
#define VITA_COREDUMP_ULTINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "defines.hpp"

namespace vita
{
namespace coredump
{

class ultInfo;

class ultInfoThread {
    friend ultInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t state();
        std::string readableState();
        uint32_t entry();
        uint32_t arg();
        uint32_t threadId();
        uint32_t exitCode();
        uint32_t fiberAddress();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _state;
        uint32_t _entry;
        uint32_t _arg;
        uint32_t _thread_id;
        uint32_t _exit_code;
        uint32_t _fiber_address;

}; // ultInfoUlt

class ultInfoRuntime {
    friend ultInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t workerThreads();
        uint32_t maxThreads();
        std::vector<ultInfoThread>& threads();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _worker_threads;
        uint32_t _max_threads;
        std::vector<ultInfoThread> _threads;

}; // ultInfoRuntime


class ultInfo {
    public:
        ultInfo(void* buf, uint32_t size);
        std::vector<ultInfoRuntime>& runtimes();

    private:
        std::vector<ultInfoRuntime> _runtimes;
}; // ultInfo

}; // coredump
}; // vita

#endif