#include "info_ultcondvarinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultCondvarInfo_info {
    uint32_t unk00; // zeroes, probably size
    uint32_t id; // addr
    char name[32];
    uint32_t attributes;
    uint32_t mutex_id;
    uint32_t num_wait_threads;
//    uint32_t thread_ids[num_wait_threads];
};

struct ultCondvarInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultCondvarInfo_info follows
};

*/

    ultCondvarInfo::ultCondvarInfo(void* buf, uint32_t size)
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
            read_var(_condvars[i]._mutex_id);
            uint32_t thread_count;
            read_var(thread_count);
            _condvars[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_condvars[i]._threads[j]);
            }
        }
    }

    std::vector<ultCondvarInfoCondvar>& ultCondvarInfo::condvars()
    {
        return _condvars;
    }

    uint32_t ultCondvarInfoCondvar::id()
    {
        return _id;
    }

    std::string& ultCondvarInfoCondvar::name()
    {
        return _name;
    }

    uint32_t ultCondvarInfoCondvar::attributes()
    {
        return _attributes;
    }

    std::string ultCondvarInfoCondvar::readableAttributes()
    {
        return std::string("NONE");
    }

    uint32_t ultCondvarInfoCondvar::mutexId()
    {
        return _mutex_id;
    }


    std::vector<ultSyncThread>& ultCondvarInfoCondvar::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

