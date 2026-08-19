#include "info_rwlockinfo.hpp"

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

struct rwlockInfo_info {
    uint32_t unk00; // size? 0x44
    uint32_t id; // 04
    uint32_t process_id; // 08
    char name[32]; // 0C
    uint32_t attributes; // 2C
    uint32_t ref_count; // 30

    uint32_t read_threads; // 34
    uint32_t write_threads; // 38
    // read_threads syncObjects_thread follow
    // write_threads syncObjects_thread follow
//    uint32_t write_owner_id; // 3C
//    uint32_t lock_count; // 40

};

struct rwlockInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of rwlockInfo_info follows
    // 0x28 zero bytes, padding?
};

*/

    rwlockInfo::rwlockInfo(void* buf, uint32_t size)
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
            read_var(_rwlocks[i]._pid);
            read_string(_rwlocks[i]._name, 32);
            read_var(_rwlocks[i]._attributes);
            read_var(_rwlocks[i]._ref_count);
            uint32_t read_thread_count;
            uint32_t write_thread_count;
            read_var(read_thread_count);
            read_var(write_thread_count);

            _rwlocks[i]._read_threads.resize(read_thread_count);
            for (uint32_t j = 0; j < read_thread_count; ++j)
            {
                read_var(_rwlocks[i]._read_threads[j].pid);
                read_var(_rwlocks[i]._read_threads[j].thread_id);
            }

            _rwlocks[i]._write_threads.resize(write_thread_count);
            for (uint32_t j = 0; j < write_thread_count; ++j)
            {
                read_var(_rwlocks[i]._write_threads[j].pid);
                read_var(_rwlocks[i]._write_threads[j].thread_id);
            }
            read_var(_rwlocks[i]._write_owner_id);
            read_var(_rwlocks[i]._lock_count);
        }
    }

    std::vector<rwlockInfoRwlock>& rwlockInfo::rwlocks()
    {
        return _rwlocks;
    }

    uint32_t rwlockInfoRwlock::id()
    {
        return _id;
    }

    uint32_t rwlockInfoRwlock::pid()
    {
        return _pid;
    }

    std::string& rwlockInfoRwlock::name()
    {
        return _name;
    }

    uint32_t rwlockInfoRwlock::attributes()
    {
        return _attributes;
    }

    std::string rwlockInfoRwlock::readableAttributes()
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
            attr.push_back("SCE_KERNEL_RW_LOCK_ATTR_RECURSIVE");
        if (_attributes & 0x4)
            attr.push_back("SCE_KERNEL_RW_LOCK_ATTR_CEILING");

        return utils::join(attr, " | ");
    }

    uint32_t rwlockInfoRwlock::refCount()
    {
        return _ref_count;
    }

    std::vector<syncObjectThread>& rwlockInfoRwlock::readThreads()
    {
        return _read_threads;
    }

    std::vector<syncObjectThread>& rwlockInfoRwlock::writeThreads()
    {
        return _write_threads;
    }

    uint32_t rwlockInfoRwlock::writeOwnerId()
    {
        return _write_owner_id;
    }

    uint32_t rwlockInfoRwlock::lockCount()
    {
        return _lock_count;
    }

}; // coredump
}; // vita

