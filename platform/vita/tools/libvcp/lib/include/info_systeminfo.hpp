#ifndef VITA_COREDUMP_SYSTEMINFO_H
#define VITA_COREDUMP_SYSTEMINFO_H

#include <cstdint>
#include <array>
#include <string>

namespace vita
{
namespace coredump
{

class systemInfo {
    public:
        systemInfo(void* buf, uint32_t size);

        std::string& versionString();
        uint32_t versionMajor();
        uint32_t versionMinor();
        uint32_t versionBuild();
        std::array<uint8_t, 16>& psid();
        std::array<uint8_t, 16>& hardwareId(); // CookedPSID
        uint32_t deviceType();
        uint32_t aslrSeed();
        bool isPSTV();
        uint64_t timeAwake(); // microseconds
        uint32_t extendedMem(); // bytes
        uint32_t vshBuild();
        uint32_t GPI();
        uint32_t qaFlags();

    private:
        std::string _version;
        uint32_t _ver_major;
        uint32_t _ver_minor;
        uint32_t _ver_build;
        std::array<uint8_t, 16> _psid;
        std::array<uint8_t, 16> _hardware_id;
        uint32_t _device_type;
        uint32_t _aslr_seed;
        bool _is_pstv;
        uint64_t _time_awake; // microseconds
        uint32_t _extended_mem; // bytes
        uint32_t _vsh_build;
        uint32_t _gpi;
        uint32_t _qa_flags;
}; // systemInfo

}; // coredump
}; // vita

#endif