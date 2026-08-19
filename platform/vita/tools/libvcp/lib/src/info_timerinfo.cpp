#include "info_timerinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct syncObjects_thread {
    uint32_t process_id;
    uint32_t thread_id;
};

struct timerInfo_info {
    uint32_t unk00; // size 0x70
    uint32_t id; // 04
    uint32_t process_id; // 08
    char name[32]; // 0C
    uint32_t attributes; // 2C
    uint32_t ref_cnt; // 30
    uint32_t active_state; // 34
    uint64_t time_base; // 38
    uint64_t time_current; // 40
    uint64_t time_scedule; // 48
    uint64_t time_interval; // 50
    uint32_t event_type; // 58
    uint32_t repeat; // 5C
    uint32_t thread_cnt; // 60
    // thread_cnt syncObjects_thread follow
//    uint32_t event_pattern; // 64
    // userdata? thread_id, process_id // 68, 6C
};


struct timerInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of timerInfo_info follows
    // 0x28 zero bytes, padding?
};

*/

    timerInfo::timerInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _timers.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_timers[i]._id);
            read_var(_timers[i]._pid);
            read_string(_timers[i]._name, 32);
            read_var(_timers[i]._attributes);
            read_var(_timers[i]._ref_count);
            read_var(_timers[i]._active_state);
            read_var(_timers[i]._time_base);
            read_var(_timers[i]._time_current);
            read_var(_timers[i]._time_schedule);
            read_var(_timers[i]._time_interval);
            read_var(_timers[i]._event_type);
            read_var(_timers[i]._repeat);

            uint32_t thread_count;
            read_var(thread_count);
            _timers[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_timers[i]._threads[j].pid);
                read_var(_timers[i]._threads[j].thread_id);
            }

            read_var(_timers[i]._event_pattern);
            read_var(_timers[i]._userdata);
        }
    }

    std::vector<timerInfoTimer>& timerInfo::timers()
    {
        return _timers;
    }

    uint32_t timerInfoTimer::id()
    {
        return _id;
    }

    uint32_t timerInfoTimer::pid()
    {
        return _pid;
    }

    std::string& timerInfoTimer::name()
    {
        return _name;
    }

    uint32_t timerInfoTimer::attributes()
    {
        return _attributes;
    }

    std::string timerInfoTimer::readableAttributes()
    {
        std::vector<std::string> attr;

        if (_attributes & 0x00080000)
            attr.push_back("SCE_UID_ATTR_OPEN_FLAG");
        if (_attributes & 0x00000080)
            attr.push_back("SCE_KERNEL_ATTR_OPENABLE");
        if (_attributes & 0x00002000)
            attr.push_back("SCE_KERNEL_ATTR_TH_PRIO");
        else
            attr.push_back("SCE_KERNEL_ATTR_TH_FIFO");
        if (_attributes & 0x00000100)
            attr.push_back("SCE_KERNEL_EVENT_ATTR_AUTO_RESET");
        else
            attr.push_back("SCE_KERNEL_EVENT_ATTR_MANUAL_RESET");

        if (_attributes & 0x00000800)
            attr.push_back("SCE_KERNEL_ATTR_NOTIFY_CB_WAKEUP_ONLY");
        else
            attr.push_back("SCE_KERNEL_ATTR_NOTIFY_CB_ALL");


        return utils::join(attr, " | ");
    }

    uint32_t timerInfoTimer::refCount()
    {
        return _ref_count;
    }

    uint32_t timerInfoTimer::activeState()
    {
        return _active_state;
    }

    uint64_t timerInfoTimer::timeBase()
    {
        return _time_base;
    }

    uint64_t timerInfoTimer::timeCurrent()
    {
        return _time_current;
    }

    uint64_t timerInfoTimer::timeSchedule()
    {
        return _time_schedule;
    }

    uint64_t timerInfoTimer::timeInterval()
    {
        return _time_interval;
    }

    uint32_t timerInfoTimer::eventType()
    {
        return _event_type;
    }

    uint32_t timerInfoTimer::repeat()
    {
        return _repeat;
    }

    std::vector<syncObjectThread>& timerInfoTimer::threads()
    {
        return _threads;
    }

    uint32_t timerInfoTimer::eventPattern()
    {
        return _event_pattern;
    }

    uint64_t timerInfoTimer::userdata()
    {
        return _userdata;
    }

}; // coredump
}; // vita

