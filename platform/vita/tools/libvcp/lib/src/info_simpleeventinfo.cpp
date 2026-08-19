#include "info_simpleeventinfo.hpp"

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

struct smplEventInfo_info {
    uint32_t unk00; // size? ox40
    uint32_t id; // 04
    uint32_t process_id; // 08
    char name[32]; // 0C
    uint32_t attributes; // 2C
    uint32_t pattern; // 30
    uint64_t userdata; // userdata? processid + threadid
    uint32_t thread_count;
    // thread_count syncObjects_thread follow
};

struct smplEventInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of smplEventInfo_info follows
    // 0x28 zero bytes, padding?
};

*/

    simpleeventInfo::simpleeventInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _events.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_events[i]._id);
            read_var(_events[i]._pid);
            read_string(_events[i]._name, 32);
            read_var(_events[i]._attributes);
            read_var(_events[i]._pattern);
            read_var(_events[i]._userdata);
            uint32_t thread_count;
            read_var(thread_count);
            _events[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_events[i]._threads[j].pid);
                read_var(_events[i]._threads[j].thread_id);
            }
        }
    }

    std::vector<simpleeventInfoEvent>& simpleeventInfo::events()
    {
        return _events;
    }

    uint32_t simpleeventInfoEvent::id()
    {
        return _id;
    }

    uint32_t simpleeventInfoEvent::pid()
    {
        return _pid;
    }

    std::string& simpleeventInfoEvent::name()
    {
        return _name;
    }

    uint32_t simpleeventInfoEvent::attributes()
    {
        return _attributes;
    }

    std::string simpleeventInfoEvent::readableAttributes()
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

    uint32_t simpleeventInfoEvent::pattern()
    {
        return _pattern;
    }

    uint64_t simpleeventInfoEvent::userdata()
    {
        return _userdata;
    }

    std::vector<syncObjectThread>& simpleeventInfoEvent::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

