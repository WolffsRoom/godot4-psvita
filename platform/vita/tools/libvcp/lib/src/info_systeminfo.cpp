#include "info_systeminfo.hpp"

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include "memstream.hpp"
#include "utils.h"


namespace vita
{
namespace coredump
{
/*
    struct systemInfo_struct { // 0x4 + 0x5C
        uint32_t unk1; // min. coredump version

        uint32_t unk2;

        uint8_t ver1[4];
        uint32_t unk3; // always 0
        uint8_t ver2[4];
        uint32_t unk4; // always 0
        uint8_t psid[16]; // ksceSblAimgrGetOpenPsId. zeroed on non-internal
        uint32_t device_type; // 1 - TOOL, 2 - DEX, 3 - CEX
        uint32_t aslr_seed;

        uint32_t unk8; // always 0
        uint32_t is_dolce;

        uint8_t hwid[16]; // CookedPSID
        uint64_t awake_time; // SceSysrootForKernel_E20F6FC8 - uptime?

        uint32_t unk11;// ksceSblQafManagerGetQAFlags. 1 if anything set? 0 otherwize?
        uint32_t vsh_build;
        uint32_t gpi;
        uint32_t additional_mem; // SceSysmemForKernel_BC36755F (3.65) / SceSysmemForKernel_4D809B47 (3.60). +109Mb(in bytes), etc
    };
*/

    systemInfo::systemInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(8, std::ios_base::beg);

        uint32_t tmp_ver;
        read_var(tmp_ver);

        std::stringstream ss;
        ss << DUMP_HEX_FORMAT(7) << tmp_ver;
        std::stringstream ss2;

        ss2 << ss.str().substr(0,1) << "." << ss.str().substr(1,3) << "." << ss.str().substr(4,3);

        _ver_major = std::stoi(ss.str().substr(0,1), nullptr, 10);
        _ver_minor = std::stoi(ss.str().substr(1,3), nullptr, 10);
        _ver_build = std::stoi(ss.str().substr(4,3), nullptr, 10);
        _version = ss2.str();

        in.seekg(12, std::ios_base::cur);
        read_array(_psid, 16);

        read_var(_device_type);
        read_var(_aslr_seed);

        in.seekg(4, std::ios_base::cur);

        uint32_t is_pstv;
        read_var(is_pstv);
        _is_pstv = is_pstv;

        read_array(_hardware_id, 16);
        read_var(_time_awake);

        read_var(_qa_flags);
        read_var(_vsh_build);
        read_var(_gpi);
        read_var(_extended_mem);
    }

    std::string& systemInfo::versionString()
    {
        return _version;
    }

    uint32_t systemInfo::versionMajor()
    {
        return _ver_major;
    }

    uint32_t systemInfo::versionMinor()
    {
        return _ver_minor;
    }

    uint32_t systemInfo::versionBuild()
    {
        return _ver_build;
    }

    std::array<uint8_t, 16>& systemInfo::psid()
    {
        return _psid;
    }

    std::array<uint8_t, 16>& systemInfo::hardwareId() // CookedPSID
    {
        return _hardware_id;
    }

    uint32_t systemInfo::deviceType()
    {
        return _device_type;
    }

    uint32_t systemInfo::aslrSeed()
    {
        return _aslr_seed;
    }

    bool systemInfo::isPSTV()
    {
        return _is_pstv;
    }

    uint64_t systemInfo::timeAwake() // microseconds
    {
        return _time_awake;
    }

    uint32_t systemInfo::extendedMem() // bytes
    {
        return _extended_mem;
    }

    uint32_t systemInfo::vshBuild()
    {
        return _vsh_build;
    }

    uint32_t systemInfo::GPI()
    {
        return _gpi;
    }

    uint32_t systemInfo::qaFlags()
    {
        return _qa_flags;
    }

}; // coredump
}; // vita

