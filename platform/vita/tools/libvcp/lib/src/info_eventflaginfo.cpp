#include "info_eventflaginfo.hpp"

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


struct eventflagInfo_eventflag {
    uint32_t unk00; // zeroes
    uint32_t id; // 04
    uint32_t process_id; // 08
    char name[32]; // 0C
    uint32_t attributes; // 2C
    uint32_t ref_count; // 30
    uint32_t initial_value; // 34
    uint32_t value; // 38
    uint32_t thread_count; // 3C
    // thread_count syncObjects_thread follow
};

struct eventflagInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of eventflagInfo_eventflag follows
};

*/

    eventflagInfo::eventflagInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _eventflags.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_eventflags[i]._id);
            read_var(_eventflags[i]._pid);
            read_string(_eventflags[i]._name, 32);
            read_var(_eventflags[i]._attributes);
            read_var(_eventflags[i]._ref_count);
            read_var(_eventflags[i]._initial_value);
            read_var(_eventflags[i]._value);
            uint32_t thread_count;
            read_var(thread_count);
            _eventflags[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_eventflags[i]._threads[j].pid);
                read_var(_eventflags[i]._threads[j].thread_id);
            }
        }
    }

    std::vector<eventflagInfoEventflag>& eventflagInfo::eventflags()
    {
        return _eventflags;
    }

    uint32_t eventflagInfoEventflag::id()
    {
        return _id;
    }

    uint32_t eventflagInfoEventflag::pid()
    {
        return _pid;
    }

    std::string& eventflagInfoEventflag::name()
    {
        return _name;
    }

    uint32_t eventflagInfoEventflag::attributes()
    {
        return _attributes;
    }

    std::string eventflagInfoEventflag::readableAttributes()
    {
        std::vector<std::string> attr;

        if (_attributes & 0x00080000)
            attr.push_back("SCE_UID_ATTR_OPEN_FLAG");
        if (_attributes & 0x00000080)
            attr.push_back("SCE_KERNEL_ATTR_OPENABLE");

        if (_attributes & 0x00001000)
            attr.push_back("SCE_KERNEL_ATTR_MULTI");
        else
            attr.push_back("SCE_KERNEL_ATTR_SINGLE");

        if (_attributes & 0x00002000)
            attr.push_back("SCE_KERNEL_ATTR_TH_PRIO");
        else
            attr.push_back("SCE_KERNEL_ATTR_TH_FIFO");


        return utils::join(attr, " | ");
    }

    uint32_t eventflagInfoEventflag::refCount()
    {
        return _ref_count;
    }

    uint32_t eventflagInfoEventflag::initialValue()
    {
        return _initial_value;
    }

    uint32_t eventflagInfoEventflag::value()
    {
        return _value;
    }

    std::vector<syncObjectThread>& eventflagInfoEventflag::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

