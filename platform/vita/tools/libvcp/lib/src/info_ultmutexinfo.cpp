#include "info_ultmutexinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultMutexInfo_info {
    uint32_t unk00; // zeroes, probably size
    uint32_t id; // addr
    char name[32];
    uint32_t attributes;
    uint32_t queue_pool_id; // addr
    uint32_t lock_status;
    uint32_t recursive_lock_count;
    uint32_t thread_owner_id;
    uint32_t num_wait_threads;
//    uint32_t thread_ids[num_wait_threads];
};

struct ultMutexInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultMutexInfo_info follows
};

*/

    ultMutexInfo::ultMutexInfo(void* buf, uint32_t size)
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
            read_var(_mutexes[i]._queue_pool_id);
            read_var(_mutexes[i]._lock_status);
            read_var(_mutexes[i]._recursive_lock_count);
            read_var(_mutexes[i]._thread_owner_id);
            uint32_t thread_count;
            read_var(thread_count);
            _mutexes[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_mutexes[i]._threads[j]);
            }
        }
    }

    std::vector<ultMutexInfoMutex>& ultMutexInfo::mutexes()
    {
        return _mutexes;
    }

    uint32_t ultMutexInfoMutex::id()
    {
        return _id;
    }

    std::string& ultMutexInfoMutex::name()
    {
        return _name;
    }

    uint32_t ultMutexInfoMutex::attributes()
    {
        return _attributes;
    }

    std::string ultMutexInfoMutex::readableAttributes()
    {
        return std::string("NONE");
    }

    uint32_t ultMutexInfoMutex::queuePoolId()
    {
        return _queue_pool_id;
    }

    uint32_t ultMutexInfoMutex::lockStatus()
    {
        return _lock_status;
    }

    uint32_t ultMutexInfoMutex::recursiveLockCount()
    {
        return _recursive_lock_count;
    }

    uint32_t ultMutexInfoMutex::threadOwnerId()
    {
        return _thread_owner_id;
    }

    std::vector<ultSyncThread>& ultMutexInfoMutex::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

