#ifndef VITA_COREDUMP_ULTQPOOLINFO_H
#define VITA_COREDUMP_ULTQPOOLINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class ultQpoolInfo;

class ultQpoolInfoPool {
    friend ultQpoolInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t maxDataResources();
        uint32_t currentDataResources();
        uint32_t dataSize();
        uint32_t maxQueueObjs();
        uint32_t currentQueueObjs();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _max_data_resources;
        uint32_t _current_data_resources;
        uint32_t _data_size;
        uint32_t _max_queue_objs;
        uint32_t _current_queue_objs;

}; // ultQpoolInfoPool


class ultQpoolInfo {
    public:
        ultQpoolInfo(void* buf, uint32_t size);
        std::vector<ultQpoolInfoPool>& pools();

    private:
        std::vector<ultQpoolInfoPool> _pools;
}; // ultQpoolInfo

}; // coredump
}; // vita

#endif