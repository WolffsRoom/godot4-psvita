#include "info_condvarinfo.hpp"

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

struct condvarInfo_info {
    uint32_t unk00; // size? 0
    uint32_t id; // 04
    uint32_t process_id; // 08
    char name[32]; // 0C
    uint32_t attributes; // 2C
    uint32_t ref_count; // 30
    uint32_t thread_id; // 34
    uint32_t thread_cnt; // 38

    // thread_cnt syncObjects_thread follow
};

struct condvarInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of condvarInfo_info follows
    // 0x28 zero bytes, padding?
};

*/

    condvarInfo::condvarInfo(void* buf, uint32_t size)
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
            read_var(_condvars[i]._pid);
            read_string(_condvars[i]._name, 32);
            read_var(_condvars[i]._attributes);
            read_var(_condvars[i]._ref_count);
            read_var(_condvars[i]._thread_id);
            uint32_t thread_count;
            read_var(thread_count);
            _condvars[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_condvars[i]._threads[j].pid);
                read_var(_condvars[i]._threads[j].thread_id);
            }
        }
    }

    std::vector<condvarInfoCondvar>& condvarInfo::condvars()
    {
        return _condvars;
    }

    uint32_t condvarInfoCondvar::id()
    {
        return _id;
    }

    uint32_t condvarInfoCondvar::pid()
    {
        return _pid;
    }

    std::string& condvarInfoCondvar::name()
    {
        return _name;
    }

    uint32_t condvarInfoCondvar::attributes()
    {
        return _attributes;
    }

    std::string condvarInfoCondvar::readableAttributes()
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

    uint32_t condvarInfoCondvar::refCount()
    {
        return _ref_count;
    }

    uint32_t condvarInfoCondvar::threadId()
    {
        return _thread_id;
    }

    std::vector<syncObjectThread>& condvarInfoCondvar::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

