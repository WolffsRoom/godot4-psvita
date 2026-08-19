#ifndef VITA_COREDUMP_EVENTFLAGINFO_H
#define VITA_COREDUMP_EVENTFLAGINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class eventflagInfo;

class eventflagInfoEventflag {
    friend eventflagInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t initialValue();
        uint32_t value();
        std::vector<syncObjectThread>& threads();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;
        uint32_t _initial_value;
        uint32_t _value;
        std::vector<syncObjectThread> _threads;

}; // eventflagInfoEventflag


class eventflagInfo {
    public:
        eventflagInfo(void* buf, uint32_t size);
        std::vector<eventflagInfoEventflag>& eventflags();

    private:
        std::vector<eventflagInfoEventflag> _eventflags;
}; // eventflagInfo

}; // coredump
}; // vita

#endif