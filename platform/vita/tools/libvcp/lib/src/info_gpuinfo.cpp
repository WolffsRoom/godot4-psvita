#include "info_gpuinfo.hpp"
#include <map>
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct gpuInfo { // size 0x3038 + 4 (on 1.6 proto), 0x3400 on 3.60
    uint32_t unk00; // coredump minver
    // data set by FUN_81009ca0 in gpu_es4
    uint32_t unk04; // ex: 10003 - pid? hardcoded
    uint32_t flags; // 0x08 ex: 0x3F - some flags ORed together

    // SceDateTime
    uint16_t year; // 0x0C
    uint16_t month; // 0x0E
    uint16_t day; // 0x10
    uint16_t hour; // 0x12
    uint16_t minute; // 0x14
    uint16_t second; // 0x16
    uint32_t microsecond; // 0x18

    uint32_t soc_revision; // 0x1C ex: 0x42 - soc revision & 0x1ffff
    uint32_t mp_freq; // 0x20 ex: 0x29 - mp freq
    uint32_t core_freq; // 0x24 ex: 0x29 - core freq
    uint32_t xbar_freq; // 0x28 ex: 0x6F - xbar freq
    uint32_t unk2C; // 0x2C ex: 0x04 - count of something
    uint32_t unk30; // 0x30 ex: 0x010A - count of something, hardcoded if exists. sets flags | 1
    uint32_t unk34; // 0x34 ex: 0x018F - count of something, hardcoded if exists. sets flags | 2
    uint32_t unk38; // 0x38 ex: 0x6D - count of something, hardcoded if exists. sets flags | 0x24

    // { uint32_t offset, uint32_t value } X unk30 follows
};

*/

    gpuInfo::gpuInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(4, std::ios_base::beg);

        read_var(_pid);
        read_var(_flags);
        read_var(_year);
        read_var(_month);
        read_var(_day);
        read_var(_hour);
        read_var(_minute);
        read_var(_second);
        read_var(_microsecond);
        read_var(_soc_revision);
        read_var(_mp_freq);
        read_var(_core_freq);
        read_var(_xbar_freq);
    }

    uint32_t gpuInfo::pid()
    {
        return _pid;
    }

    uint32_t gpuInfo::flags()
    {
        return _flags;
    }

    uint16_t gpuInfo::year()
    {
        return _year;
    }

    uint16_t gpuInfo::month()
    {
        return _month;
    }

    uint16_t gpuInfo::day()
    {
        return _day;
    }

    uint16_t gpuInfo::hour()
    {
        return _hour;
    }

    uint16_t gpuInfo::minute()
    {
        return _minute;
    }

    uint16_t gpuInfo::second()
    {
        return _second;
    }

    uint32_t gpuInfo::microsecond()
    {
        return _microsecond;
    }

    uint32_t gpuInfo::socRevision()
    {
        return _soc_revision;
    }

    uint32_t gpuInfo::mpFreq()
    {
        return _mp_freq;
    }

    uint32_t gpuInfo::coreFreq()
    {
        return _core_freq;
    }

    uint32_t gpuInfo::xbarFreq()
    {
        return _xbar_freq;
    }

}; // coredump
}; // vita

