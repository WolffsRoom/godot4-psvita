#include "info_ultsemaphoreinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultSemaInfo_info {
    uint32_t unk00; // zeroes, probably size
    uint32_t id; // addr
    char name[32];
    uint32_t attributes;
    uint32_t queue_pool_id; // addr

    uint32_t num_current_resources;

    uint32_t num_wait_threads;

//    uint32_t thread_ids[num_wait_threads];

};

struct ultSemaInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultSemaInfo_info follows
};
*/

    ultSemaphoreInfo::ultSemaphoreInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _semaphores.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_semaphores[i]._id);
            read_string(_semaphores[i]._name, 32);
            read_var(_semaphores[i]._attributes);
            read_var(_semaphores[i]._queue_pool_id);
            read_var(_semaphores[i]._current_resources);
            uint32_t thread_count;
            read_var(thread_count);
            _semaphores[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                read_var(_semaphores[i]._threads[j]);
            }
        }
    }

    std::vector<ultSemaphoreInfoSemaphore>& ultSemaphoreInfo::semaphores()
    {
        return _semaphores;
    }

    uint32_t ultSemaphoreInfoSemaphore::id()
    {
        return _id;
    }

    std::string& ultSemaphoreInfoSemaphore::name()
    {
        return _name;
    }

    uint32_t ultSemaphoreInfoSemaphore::attributes()
    {
        return _attributes;
    }

    std::string ultSemaphoreInfoSemaphore::readableAttributes()
    {
        return std::string("NONE");
    }

    uint32_t ultSemaphoreInfoSemaphore::currentResources()
    {
        return _current_resources;
    }

    uint32_t ultSemaphoreInfoSemaphore::queuePoolId()
    {
        return _queue_pool_id;
    }


    std::vector<ultSyncThread>& ultSemaphoreInfoSemaphore::threads()
    {
        return _threads;
    }

}; // coredump
}; // vita

