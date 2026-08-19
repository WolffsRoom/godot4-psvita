#include "info_mutexinfo.hpp"

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

struct mutexInfo_mutex {
    uint32_t unk00; // zeroes
    uint32_t id; // 04
    uint32_t process_id; // 08
    char name[32]; // 0C
    uint32_t attributes; // 2C
    uint32_t ref_count; // 30
    uint32_t initial_value; // 34
    uint32_t value; // 38
    uint32_t thread_owner_id; // 3C
    uint32_t thread_count; // 40
    uint32_t ceiling_priority; // 44 ?
    // thread_count syncObjects_thread follow
};

struct mutexInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of mutexInfo_mutex follows
};


*/

    mutexInfo::mutexInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _mutexes.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_mutexes[i]._id);
            read_var(_mutexes[i]._pid);
            read_string(_mutexes[i]._name, 32);
            read_var(_mutexes[i]._attributes);
            read_var(_mutexes[i]._ref_count);
            read_var(_mutexes[i]._initial_value);
            read_var(_mutexes[i]._value);
            read_var(_mutexes[i]._thread_owner_id);

            uint32_t thread_count;
            read_var(thread_count);

            read_var(_mutexes[i]._ceiling_priority);

            _mutexes[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_mutexes[i]._threads[j].pid);
                read_var(_mutexes[i]._threads[j].thread_id);
            }
        }
    }

    std::vector<mutexInfoMutex>& mutexInfo::mutexes()
    {
        return _mutexes;
    }

    uint32_t mutexInfoMutex::id()
    {
        return _id;
    }

    uint32_t mutexInfoMutex::pid()
    {
        return _pid;
    }

    std::string& mutexInfoMutex::name()
    {
        return _name;
    }

    uint32_t mutexInfoMutex::attributes()
    {
        return _attributes;
    }

    std::string mutexInfoMutex::readableAttributes()
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

        if (_attributes & 0x2)
            attr.push_back("SCE_KERNEL_MUTEX_ATTR_RECURSIVE");
        if (_attributes & 0x4)
            attr.push_back("SCE_KERNEL_MUTEX_ATTR_CEILING");

        return utils::join(attr, " | ");
    }

    uint32_t mutexInfoMutex::refCount()
    {
        return _ref_count;
    }

    uint32_t mutexInfoMutex::initialValue()
    {
        return _initial_value;
    }

    uint32_t mutexInfoMutex::value()
    {
        return _value;
    }

    uint32_t mutexInfoMutex::threadOwnerId()
    {
        return _thread_owner_id;
    }

    uint32_t mutexInfoMutex::ceilingPriority()
    {
        return _ceiling_priority;
    }

    std::vector<syncObjectThread>& mutexInfoMutex::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

