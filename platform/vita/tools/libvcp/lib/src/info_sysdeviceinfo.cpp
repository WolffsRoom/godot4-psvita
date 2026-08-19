#include "info_sysdeviceinfo.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct sysDeviceInfo_struct { // 0x8 + 0x40 + 0x70
      uint32_t unk1; // min. coredump version

      struct {
        uint32_t struct_size; // 0x40
        uint32_t unk04; // 0x24  (title + this field?)
        char title[32];

        uint32_t unk28; // always 0x00010014. card type?

        uint32_t card_present;

        uint8_t id[16];
      } gameCard;

      struct  {
        uint32_t struct_size; // 0x70
        uint32_t unk04; // 0x00000024 (title + this field?)
        char title[32];

        uint32_t unk28; // always 0x00010044. card type?

        uint32_t card_present; // 2C

        // copy of SceMsInfo struct
        uint32_t unk30; // 0,1,2,3,4,5. Only 5 spotted
        uint32_t unk34; // always 0?
        uint64_t nbytes; // 38
        uint64_t nbytes2; // 40
        uint32_t sector_size; // 48 - always 0x200
        uint32_t unk4C; // always 0
        uint32_t fs_offset; // 50
        uint32_t unk54;
        uint32_t unk58;
        uint32_t unk5C;
        uint8_t id[0x10]; // coredump copies only first part of MCID
      } memoryCard;
    };
*/

    std::string& sysDeviceInfo_mc::title()
    {
        return _title;
    }

    uint64_t sysDeviceInfo_mc::size()
    {
        return _size;
    }

    bool sysDeviceInfo_mc::present()
    {
        return _present;
    }

    std::string sysDeviceInfo_mc::id()
    {
        std::ostringstream s;
        for (std::size_t i = 0; i<16; i++)
        {
            s << std::setw( 2 ) << std::setfill( '0' ) << std::hex << std::right << static_cast<uint32_t>(_id[i]);
        }
        return s.str();
    }

    std::array<uint8_t, 16>& sysDeviceInfo_mc::rawid()
    {
        return _id;
    }


    std::string& sysDeviceInfo_gc::title()
    {
        return _title;
    }

    bool sysDeviceInfo_gc::present()
    {
        return _present;
    }

    std::string sysDeviceInfo_gc::id()
    {
        std::ostringstream s;
        for (std::size_t i = 0; i<16; i++)
        {
            s << std::setw( 2 ) << std::setfill( '0' ) << std::hex << std::right << static_cast<uint32_t>(_id[i]);
        }
        return s.str();
    }

    sysDeviceInfo::sysDeviceInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(4, std::ios_base::beg); // to gamecard info

        // gc
        in.seekg(8, std::ios_base::cur);
        read_string(_gc._title, 32);
        in.seekg(4, std::ios_base::cur);
        uint32_t present;
        read_var(present);
        _gc._present = present;
        read_array(_gc._id, 16);

        //mc
        in.seekg(8, std::ios_base::cur);
        read_string(_mc._title, 32);

        in.seekg(4, std::ios_base::cur);
        read_var(present);
        _mc._present = present;

        in.seekg(8, std::ios_base::cur);
        read_var(_mc._size);

        in.seekg(32, std::ios_base::cur);
        read_array(_mc._id, 16);
    }

    sysDeviceInfo_mc& sysDeviceInfo::mc()
    {
        return _mc;
    }

    sysDeviceInfo_gc& sysDeviceInfo::gc()
    {
        return _gc;
    }

}; // coredump
}; // vita

