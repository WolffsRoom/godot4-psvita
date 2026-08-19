#include "info_ultrwlockinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultRWLockInfo_info {
    uint32_t size; // 0x44
    uint32_t id; // addr
    char name[32];
    uint32_t attributes;
    uint32_t queue_pool_id; // addr
    uint32_t lock_status;
    uint32_t num_locking_readers;

    uint32_t num_read_threads;
    uint32_t num_write_threads;
//    uint32_t read_thread_ids[num_read_threads];
//    uint32_t write_thread_ids[num_write_threads];
//    uint32_t thread_owner_id;
};

struct ultRWLockInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultRWLockInfo_info follows
};
*/

    ultRwlockInfo::ultRwlockInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _rwlocks.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_rwlocks[i]._id);
            read_string(_rwlocks[i]._name, 32);
            read_var(_rwlocks[i]._attributes);
            read_var(_rwlocks[i]._queue_pool_id);
            read_var(_rwlocks[i]._lock_status);
            read_var(_rwlocks[i]._locking_readers);
            uint32_t rthread_count;
            read_var(rthread_count);
            uint32_t wthread_count;
            read_var(wthread_count);

            _rwlocks[i]._read_threads.resize(rthread_count);
            for (uint32_t j = 0; j < rthread_count; ++j)
            {
                read_var(_rwlocks[i]._read_threads[j]);
            }

            _rwlocks[i]._write_threads.resize(wthread_count);
            for (uint32_t j = 0; j < wthread_count; ++j)
            {
                read_var(_rwlocks[i]._write_threads[j]);
            }

            read_var(_rwlocks[i]._thread_owner_id);
        }
    }

    std::vector<ultRwlockInfoRwlock>& ultRwlockInfo::rwlocks()
    {
        return _rwlocks;
    }

    uint32_t ultRwlockInfoRwlock::id()
    {
        return _id;
    }

    std::string& ultRwlockInfoRwlock::name()
    {
        return _name;
    }

    uint32_t ultRwlockInfoRwlock::attributes()
    {
        return _attributes;
    }

    std::string ultRwlockInfoRwlock::readableAttributes()
    {
        return std::string("NONE");
    }


    uint32_t ultRwlockInfoRwlock::queuePoolId()
    {
        return _queue_pool_id;
    }

    uint32_t ultRwlockInfoRwlock::lockStatus()
    {
        return _lock_status;
    }

    uint32_t ultRwlockInfoRwlock::lockingReaders()
    {
        return _locking_readers;
    }

    uint32_t ultRwlockInfoRwlock::threadOwnerId()
    {
        return _thread_owner_id;
    }

    std::vector<ultSyncThread>& ultRwlockInfoRwlock::readThreads()
    {
        return _read_threads;
    }

    std::vector<ultSyncThread>& ultRwlockInfoRwlock::writeThreads()
    {
        return _write_threads;
    }

}; // coredump
}; // vita

