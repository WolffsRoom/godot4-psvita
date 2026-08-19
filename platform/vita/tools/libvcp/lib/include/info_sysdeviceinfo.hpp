#ifndef VITA_COREDUMP_SYSDEVICEINFO_H
#define VITA_COREDUMP_SYSDEVICEINFO_H

#include <cstdint>
#include <string>
#include <array>

namespace vita
{
namespace coredump
{

class sysDeviceInfo;

class sysDeviceInfo_gc {
    friend sysDeviceInfo;
    public:
        std::string id();
        std::string& title();
        bool present();
    private:
        std::string _title;
        bool _present;
        std::array<uint8_t, 16> _id;
};

class sysDeviceInfo_mc {
    friend sysDeviceInfo;
    public:
        std::string id();
        std::array<uint8_t, 16>& rawid();
        std::string& title();
        bool present();
        uint64_t size();
    private:
        std::string _title;
        bool _present;
        std::array<uint8_t, 16> _id;
        uint64_t _size;
};


class sysDeviceInfo {
    public:
        sysDeviceInfo(void* buf, uint32_t size);

        sysDeviceInfo_gc& gc();
        sysDeviceInfo_mc& mc();

    private:
        sysDeviceInfo_gc _gc;
        sysDeviceInfo_mc _mc;
}; // sysDeviceInfo

}; // coredump
}; // vita

#endif