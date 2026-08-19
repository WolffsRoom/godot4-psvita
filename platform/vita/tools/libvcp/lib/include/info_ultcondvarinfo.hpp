#ifndef VITA_COREDUMP_ULTCONDVARINFO_H
#define VITA_COREDUMP_ULTCONDVARINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class ultCondvarInfo;

class ultCondvarInfoCondvar {
    friend ultCondvarInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t mutexId();
        std::vector<ultSyncThread>& threads();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _mutex_id;
        std::vector<ultSyncThread> _threads;

}; // ultCondvarInfoCondvar


class ultCondvarInfo {
    public:
        ultCondvarInfo(void* buf, uint32_t size);
        std::vector<ultCondvarInfoCondvar>& condvars();

    private:
        std::vector<ultCondvarInfoCondvar> _condvars;
}; // ultCondvarInfo

}; // coredump
}; // vita

#endif