#include "info_appinfo.hpp"

#include <sstream>
#include <iomanip>
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct appInfo_struct { // 0x4 + 0x100
        uint32_t unk1; // min. coredump version

        uint32_t unk2; // zeroes
        char titleId[10];
        char title[128];
        char version[6];
        uint8_t sdk_version[4];
        char unk3[104]; // SceProcessmgrForDriver_D141C076
    };
*/

    appInfo::appInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(8, std::ios_base::beg);

        read_string(_titleId, 10);
        read_string(_title, 128);
        read_string(_version, 6);

        uint8_t sdk_version[4];
        read_var(sdk_version[0]);
        read_var(sdk_version[1]);
        read_var(sdk_version[2]);
        read_var(sdk_version[3]);

        std::ostringstream ss;
        ss << std::hex
           << (uint32_t)(sdk_version[3]) << "."
           << (uint32_t)(sdk_version[2]) << "."
           << (uint32_t)(sdk_version[1]) << "."
           << (uint32_t)(sdk_version[0]);
        _sdkVersion = ss.str();
    }

    std::string& appInfo::title()
    {
        return _title;
    }

    std::string& appInfo::titleId()
    {
        return _titleId;
    }

    std::string& appInfo::version()
    {
        return _version;
    }

    std::string& appInfo::sdkVersion()
    {
        return _sdkVersion;
    }

}; // coredump
}; // vita

