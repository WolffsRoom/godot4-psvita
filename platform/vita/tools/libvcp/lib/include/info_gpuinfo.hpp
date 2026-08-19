#ifndef VITA_COREDUMP_GPUINFO_H
#define VITA_COREDUMP_GPUINFO_H

#include <cstdint>
#include <string>

namespace vita
{
namespace coredump
{


class gpuInfo {
    public:
        gpuInfo(void* data, uint32_t size);
        uint32_t pid();
        uint32_t flags();

        uint16_t year();
        uint16_t month();
        uint16_t day();
        uint16_t hour();
        uint16_t minute();
        uint16_t second();
        uint32_t microsecond();

        uint32_t socRevision();
        uint32_t mpFreq();
        uint32_t coreFreq();
        uint32_t xbarFreq();

    private:
        uint32_t _pid;
        uint32_t _flags;

        uint16_t _year;
        uint16_t _month;
        uint16_t _day;
        uint16_t _hour;
        uint16_t _minute;
        uint16_t _second;
        uint32_t _microsecond;

        uint32_t _soc_revision;
        uint32_t _mp_freq;
        uint32_t _core_freq;
        uint32_t _xbar_freq;

}; // gpuInfo

}; // coredump
}; // vita

#endif