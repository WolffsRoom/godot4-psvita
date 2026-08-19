#include "info_lwcondvarinfo.hpp"

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

struct lwcondvarInfo_condvar {
    uint32_t unk00; // zeroes
    uint32_t id; // 04
    char name[32]; // 08
    uint32_t attributes; // 28

    uint32_t ref_cnt; // 2C - always set to 0?
    uint32_t mutex_id; // 30
    uint32_t thread_count; // 34
    // thread_count syncObjects_thread follow
//    uint32_t work_addr; // 38
//    uint32_t mutex_work_addr; // 3C
};

struct lwcondvarInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of lwcondvarInfo_condvar follows
    // 0x28 zero bytes. padding?
};


*/

    lwcondvarInfo::lwcondvarInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _condvars.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_condvars[i]._id);
            read_string(_condvars[i]._name, 32);
            read_var(_condvars[i]._attributes);
            read_var(_condvars[i]._ref_count);
            read_var(_condvars[i]._mutex_id);
            uint32_t thread_count;
            read_var(thread_count);
            _condvars[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_condvars[i]._threads[j].pid);
                read_var(_condvars[i]._threads[j].thread_id);
            }
            read_var(_condvars[i]._work_address);
            read_var(_condvars[i]._mutex_work_address);
        }
    }

    std::vector<lwcondvarInfoCondvar>& lwcondvarInfo::condvars()
    {
        return _condvars;
    }

    uint32_t lwcondvarInfoCondvar::id()
    {
        return _id;
    }

    std::string& lwcondvarInfoCondvar::name()
    {
        return _name;
    }

    uint32_t lwcondvarInfoCondvar::attributes()
    {
        return _attributes;
    }

    std::string lwcondvarInfoCondvar::readableAttributes()
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

        return utils::join(attr, " | ");
    }

    uint32_t lwcondvarInfoCondvar::refCount()
    {
        return _ref_count;
    }

    uint32_t lwcondvarInfoCondvar::mutexId()
    {
        return _mutex_id;
    }

    uint32_t lwcondvarInfoCondvar::workAddress()
    {
        return _work_address;
    }

    uint32_t lwcondvarInfoCondvar::mutexWorkAddress()
    {
        return _mutex_work_address;
    }

    std::vector<syncObjectThread>& lwcondvarInfoCondvar::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

