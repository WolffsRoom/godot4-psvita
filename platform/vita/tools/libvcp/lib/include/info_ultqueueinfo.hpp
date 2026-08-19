#ifndef VITA_COREDUMP_ULTQUEUEINFO_H
#define VITA_COREDUMP_ULTQUEUEINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class ultQueueInfo;

class ultQueueInfoQueue {
    friend ultQueueInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();

        uint32_t writeQueuePoolId();
        uint32_t queuePoolId();
        uint32_t dataSize();

        std::vector<ultSyncThread>& pushThreads();
        std::vector<ultSyncThread>& popThreads();
        std::vector<uint8_t>& data();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;

        uint32_t _write_queue_pool_id;
        uint32_t _queue_pool_id;
        uint32_t _data_size;

        std::vector<ultSyncThread> _push_threads;
        std::vector<ultSyncThread> _pop_threads;
        std::vector<uint8_t> _data;

}; // ultQueueInfoQueue


class ultQueueInfo {
    public:
        ultQueueInfo(void* buf, uint32_t size);
        std::vector<ultQueueInfoQueue>& queues();

    private:
        std::vector<ultQueueInfoQueue> _queues;
}; // ultQueueInfo

}; // coredump
}; // vita

#endif