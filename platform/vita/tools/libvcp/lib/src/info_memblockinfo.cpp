#include "info_memblockinfo.hpp"

#include <cstdint>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include "utils.h"
#include "defines.hpp"
#include "memstream.hpp"
#include <iostream>

namespace vita
{
namespace coredump
{
/*
struct memBlkInfo_info { // size 0x48
    uint32_t unk00; // zeroes
    uint32_t id; // 04
    char name[32]; //08
    uint32_t type; //28
    uint32_t header_address; //2C
    uint32_t block_size; // 30
    uint32_t unk1; // 34 - zeroes
    uint32_t unk2; // 38 - zeroes
    uint32_t allocated_size; // 3C
    uint32_t low_size; // 40
    uint32_t high_size; // 44
};

struct memBlkInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of memBlkInfo_info follows
};

*/

    memblockInfo::memblockInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _memblocks.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_memblocks[i]._id);
            read_string(_memblocks[i]._name, 32);
            read_var(_memblocks[i]._type);
            read_var(_memblocks[i]._header_address);
            read_var(_memblocks[i]._block_size);

            in.seekg(8, std::ios_base::cur);

            read_var(_memblocks[i]._allocated_size);
            read_var(_memblocks[i]._low_size);
            read_var(_memblocks[i]._high_size);

        }
    }

    std::vector<memblockInfoBlock>& memblockInfo::memblocks()
    {
        return _memblocks;
    }


    uint32_t memblockInfoBlock::id()
    {
        return _id;
    }

    std::string& memblockInfoBlock::name()
    {
        return _name;
    }

    uint32_t memblockInfoBlock::type()
    {
        return _type;
    }

    std::string memblockInfoBlock::readableType()
    {
        switch ((memblockType)_type) {

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_FREE:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_FREE");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_MAIN_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_MAIN_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_MAIN_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_MAIN_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_MAIN_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_MAIN_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_SHARED_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_CDIALOG_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_CDIALOG_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_CDIALOG_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_CDIALOG_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_CDIALOG_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_CDIALOG_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_TOOL_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_TOOL_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_TOOL_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_TOOL_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_TOOL_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_TOOL_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM2_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM2_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_KTMP_NC_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_NC_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_CODE_USER_CDRAM_L1WBWA_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_CODE_USER_CDRAM_L1WBWA_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_IO_DEVICE_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_IO_DEVICE_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_IO_SO_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_IO_SO_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_DEVICE_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_DEVICE_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_NC_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_GAME_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_GAME_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_NC_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_TOOL_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_TOOL_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_TOOL_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_TOOL_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_TOOL_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_TOOL_NC_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_R");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_CDIALOG_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_CDIALOG_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_NC_RW");


            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_SO_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_SO_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_DEVICE_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_DEVICE_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_NC_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_BOOT_IMAGE:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_BOOT_IMAGE");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_GAME_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_GAME_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_UMAIN_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_UMAIN_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_CDIALOG_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_CDIALOG_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_CDIALOG_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_CDIALOG_R");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_RX:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_RX");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TOOL_NC_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_DEVICE_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_DEVICE_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_DEVICE_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_DEVICE_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_SO_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_SO_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_SO_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_SO_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_IO_NC_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_ROOT_PHYCONT_NC_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_CDRAM_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_CDRAM_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_CDRAM_L1WBWA_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_CDRAM_L1WBWA_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_CDRAM2_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_CDRAM2_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_NC_R:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_NC_R");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_NC_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_NC_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_GAME_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_TMP_GAME_RW");

            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_GPU_CDRAM_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_GPU_CDRAM_RW");
            case vita::coredump::memblockType::SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_KMP_TOOL_RW:
                return std::string("SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_KMP_TOOL_RW");

        }
        return std::string("UNKNOWN");
    }

    uint32_t memblockInfoBlock::headerAddress()
    {
        return _header_address;
    }

    uint32_t memblockInfoBlock::blockSize()
    {
        return _block_size;
    }

    uint32_t memblockInfoBlock::allocatedSize()
    {
        return _allocated_size;
    }

    uint32_t memblockInfoBlock::lowSize()
    {
        return _low_size;
    }

    uint32_t memblockInfoBlock::highSize()
    {
        return _high_size;
    }

}; // coredump
}; // vita

