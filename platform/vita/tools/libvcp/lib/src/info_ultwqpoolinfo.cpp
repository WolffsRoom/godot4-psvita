#include "info_ultwqpoolinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultWQPoolInfo_info {
    uint32_t size; // 0x44
    uint32_t id; // addr
    char name[32];
    uint32_t max_num_waiters;
    uint32_t num_waiters;
    uint32_t max_sync_objs;
    uint32_t num_sync_objs;
};

struct ultWQPoolInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultWQPoolInfo_info follows
    // padding, 5xuint32_t
};

*/

    ultWqpoolInfo::ultWqpoolInfo(void* buf, uint32_t size)
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
            read_var(_pools[i]._max_waiters);
            read_var(_pools[i]._current_waiters);
            read_var(_pools[i]._max_sync_objs);
            read_var(_pools[i]._current_sync_objs);

        }
    }

    std::vector<ultWqpoolInfoPool>& ultWqpoolInfo::pools()
    {
        return _pools;
    }

    uint32_t ultWqpoolInfoPool::id()
    {
        return _id;
    }

    std::string& ultWqpoolInfoPool::name()
    {
        return _name;
    }

    uint32_t ultWqpoolInfoPool::maxWaiters()
    {
        return _max_waiters;
    }

    uint32_t ultWqpoolInfoPool::currentWaiters()
    {
        return _current_waiters;
    }

    uint32_t ultWqpoolInfoPool::maxSyncObjs()
    {
        return _max_sync_objs;
    }

    uint32_t ultWqpoolInfoPool::currentSyncObjs()
    {
        return _current_sync_objs;
    }


}; // coredump
}; // vita

