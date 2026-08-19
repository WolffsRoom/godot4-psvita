#ifndef VITA_COREDUMP_MEMBLOCKINFO_H
#define VITA_COREDUMP_MEMBLOCKINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace vita
{
namespace coredump
{

class memblockInfo;

class memblockInfoBlock {
    friend memblockInfo;
    public:

        uint32_t id();
        std::string& name();
        uint32_t type();
        std::string readableType();
        uint32_t headerAddress();
        uint32_t blockSize();
        uint32_t allocatedSize();
        uint32_t lowSize();
        uint32_t highSize();
    private:
        uint32_t _id;
        std::string _name;
        uint32_t _type;
        uint32_t _header_address;
        uint32_t _block_size;
        uint32_t _allocated_size;
        uint32_t _low_size;
        uint32_t _high_size;
}; // memblockInfoBlock


class memblockInfo {
    public:
        memblockInfo(void* buf, uint32_t size);
        std::vector<memblockInfoBlock>& memblocks();

    private:
        std::vector<memblockInfoBlock> _memblocks;
}; // memblockInfo

}; // coredump
}; // vita

#endif