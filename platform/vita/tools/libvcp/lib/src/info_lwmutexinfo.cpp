#include "info_lwmutexinfo.hpp"

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


struct lwMutexInfo_mutex {
    uint32_t unk00; // zeroes
    uint32_t id; // 04
    char name[32]; // 08
    uint32_t attributes; // 28

    uint32_t work_address; // 2C
    uint32_t initial_value; // 30
    uint32_t value; // 34
    uint32_t thread_owner_id; // 38
    uint32_t thread_count; // 3C
    // thread_count syncObjects_thread follow
};


struct lwMutexInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of lwMutexInfo_mutex follows
};


*/

    lwmutexInfo::lwmutexInfo(void* buf, uint32_t size)
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
            read_string(_mutexes[i]._name, 32);
            read_var(_mutexes[i]._attributes);
            read_var(_mutexes[i]._work_address);
            read_var(_mutexes[i]._initial_value);
            read_var(_mutexes[i]._value);
            read_var(_mutexes[i]._thread_owner_id);

            uint32_t thread_count;
            read_var(thread_count);


            _mutexes[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_mutexes[i]._threads[j].pid);
                read_var(_mutexes[i]._threads[j].thread_id);
            }
        }
    }

    std::vector<lwmutexInfoMutex>& lwmutexInfo::mutexes()
    {
        return _mutexes;
    }

    uint32_t lwmutexInfoMutex::id()
    {
        return _id;
    }

    std::string& lwmutexInfoMutex::name()
    {
        return _name;
    }

    uint32_t lwmutexInfoMutex::attributes()
    {
        return _attributes;
    }

    std::string lwmutexInfoMutex::readableAttributes()
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

    uint32_t lwmutexInfoMutex::workAddress()
    {
        return _work_address;
    }

    uint32_t lwmutexInfoMutex::initialValue()
    {
        return _initial_value;
    }

    uint32_t lwmutexInfoMutex::value()
    {
        return _value;
    }

    uint32_t lwmutexInfoMutex::threadOwnerId()
    {
        return _thread_owner_id;
    }


    std::vector<syncObjectThread>& lwmutexInfoMutex::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

