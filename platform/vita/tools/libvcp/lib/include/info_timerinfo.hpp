#ifndef VITA_COREDUMP_TIMERINFO_H
#define VITA_COREDUMP_TIMERINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class timerInfo;

class timerInfoTimer {
    friend timerInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t activeState();
        uint64_t timeBase();
        uint64_t timeCurrent();
        uint64_t timeSchedule();
        uint64_t timeInterval();
        uint32_t eventType();
        uint32_t repeat();
        std::vector<syncObjectThread>& threads();
        uint32_t eventPattern();
        uint64_t userdata();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;

        uint32_t _active_state;
        uint64_t _time_base;
        uint64_t _time_current;
        uint64_t _time_schedule;
        uint64_t _time_interval;
        uint32_t _event_type;
        uint32_t _repeat;
        std::vector<syncObjectThread> _threads;
        uint32_t _event_pattern;
        uint64_t _userdata;

}; // timerInfoTimer


class timerInfo {
    public:
        timerInfo(void* buf, uint32_t size);
        std::vector<timerInfoTimer>& timers();

    private:
        std::vector<timerInfoTimer> _timers;
}; // timerInfo

}; // coredump
}; // vita

#endif