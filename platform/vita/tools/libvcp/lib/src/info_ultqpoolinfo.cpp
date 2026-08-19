#include "info_ultqpoolinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultQueuePoolInfo_info {
    uint32_t size; // 0x44
    uint32_t id; // addr
    char name[32];

    uint32_t attributes;

    uint32_t max_data_resources;
    uint32_t num_data_resources;

    uint32_t data_size;

    uint32_t max_queue_objects;
    uint32_t num_queue_objects;
};

struct ultQueuePoolInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultQUeuePoolInfo_info follows
    // padding, 5xuint32_t
};

*/

    ultQpoolInfo::ultQpoolInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _pools.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_pools[i]._id);
            read_string(_pools[i]._name, 32);
            read_var(_pools[i]._attributes);
            read_var(_pools[i]._max_data_resources);
            read_var(_pools[i]._current_data_resources);
            read_var(_pools[i]._data_size);
            read_var(_pools[i]._max_queue_objs);
            read_var(_pools[i]._current_queue_objs);
        }
    }

    std::vector<ultQpoolInfoPool>& ultQpoolInfo::pools()
    {
        return _pools;
    }

    uint32_t ultQpoolInfoPool::id()
    {
        return _id;
    }

    std::string& ultQpoolInfoPool::name()
    {
        return _name;
    }

    uint32_t ultQpoolInfoPool::attributes()
    {
        return _attributes;
    }

    std::string ultQpoolInfoPool::readableAttributes()
    {
        return std::string("NONE");
    }

    uint32_t ultQpoolInfoPool::dataSize()
    {
        return _data_size;
    }

    uint32_t ultQpoolInfoPool::maxDataResources()
    {
        return _max_data_resources;
    }

    uint32_t ultQpoolInfoPool::currentDataResources()
    {
        return _current_data_resources;
    }

    uint32_t ultQpoolInfoPool::maxQueueObjs()
    {
        return _max_queue_objs;
    }

    uint32_t ultQpoolInfoPool::currentQueueObjs()
    {
        return _current_queue_objs;
    }


}; // coredump
}; // vita

