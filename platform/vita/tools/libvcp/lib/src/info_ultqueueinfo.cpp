#include "info_ultqueueinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultQueueInfo_info {
    uint32_t size; // 0x44
    uint32_t id; // addr
    char name[32];

    uint32_t attributes;

    uint32_t wq_pool_id;
    uint32_t queue_data_pool_id;
    uint32_t data_size;

    uint32_t num_push_threads;
    uint32_t num_pop_threads;
    uint32_t data_count;
//    uint32_t push_thread_ids[num_push_threads];
//    uint32_t pop_thread_ids[num_pop_threads];
//    data_count * uint8_t[data_size]
};

struct ultQueueInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultQueueInfo_info follows
    // padding, 5xuint32_t
};


*/

    ultQueueInfo::ultQueueInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _queues.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_queues[i]._id);
            read_string(_queues[i]._name, 32);
            read_var(_queues[i]._attributes);

            read_var(_queues[i]._write_queue_pool_id);
            read_var(_queues[i]._queue_pool_id);
            read_var(_queues[i]._data_size);

            uint32_t push_thread_count;
            read_var(push_thread_count);

            uint32_t pop_thread_count;
            read_var(pop_thread_count);

            uint32_t data_count;
            read_var(data_count);

            uint32_t real_data_size = ((_queues[i]._data_size * data_count) + 3) & ~0x03; // align to 4

            _queues[i]._push_threads.resize(push_thread_count);
            for (uint32_t j = 0; j < push_thread_count; ++j)
            {
                read_var(_queues[i]._push_threads[j]);
            }

            _queues[i]._pop_threads.resize(pop_thread_count);
            for (uint32_t j = 0; j < pop_thread_count; ++j)
            {
                read_var(_queues[i]._pop_threads[j]);
            }

            uint32_t data_size = _queues[i]._data_size * data_count;
            _queues[i]._data.resize(data_size);

            for (uint32_t j = 0; j < data_size; ++j)
            {
                read_var(_queues[i]._data[j]);
            }
            in.seekg(real_data_size - data_size, std::ios_base::cur);

        }
    }

    std::vector<ultQueueInfoQueue>& ultQueueInfo::queues()
    {
        return _queues;
    }

    uint32_t ultQueueInfoQueue::id()
    {
        return _id;
    }

    std::string& ultQueueInfoQueue::name()
    {
        return _name;
    }

    uint32_t ultQueueInfoQueue::attributes()
    {
        return _attributes;
    }

    std::string ultQueueInfoQueue::readableAttributes()
    {
        return std::string("NONE");
    }


    uint32_t ultQueueInfoQueue::writeQueuePoolId()
    {
        return _write_queue_pool_id;
    }

    uint32_t ultQueueInfoQueue::queuePoolId()
    {
        return _queue_pool_id;
    }

    uint32_t ultQueueInfoQueue::dataSize()
    {
        return _data_size;
    }

    std::vector<ultSyncThread>& ultQueueInfoQueue::pushThreads()
    {
        return _push_threads;
    }

    std::vector<ultSyncThread>& ultQueueInfoQueue::popThreads()
    {
        return _pop_threads;
    }

    std::vector<uint8_t>& ultQueueInfoQueue::data()
    {
        return _data;
    }

}; // coredump
}; // vita

