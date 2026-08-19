#include "info_buildverinfo.hpp"
#include <map>
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct buildVerInfo_struct { // 0x4 + 0x200
      uint32_t unk1; // min. coredump version

      uint32_t unk2;

      uint32_t sys_rev;
      uint32_t vsh_rev;
      uint32_t syscon_rev; // or version?

      char sys_branch[64];
      char vsh_branch[64];
      char syscon_branch[64];
      uint32_t sdk_internal_build;
      uint32_t vsh_build;

      uint8_t unk3[0x128]; // some reserved area? always zeroes
    };
*/

    buildVerInfo::buildVerInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(8, std::ios_base::beg);

        read_var(_system_revision);
        read_var(_vsh_revision);
        read_var(_syscon_revision);

        read_string(_system_branch, 64);
        read_string(_vsh_branch, 64);
        read_string(_syscon_branch, 64);

        read_var(_sdk_internal_build);
        read_var(_vsh_build);
    }


    uint32_t buildVerInfo::systemRevision()
    {
        return _system_revision;
    }

    uint32_t buildVerInfo::vshRevision()
    {
        return _vsh_revision;
    }

    uint32_t buildVerInfo::sysconRevision()
    {
        return _syscon_revision;
    }

    std::string& buildVerInfo::systemBranch()
    {
        return _system_branch;
    }

    std::string& buildVerInfo::vshBranch()
    {
        return _vsh_branch;
    }

    std::string& buildVerInfo::sysconBranch()
    {
        return _syscon_branch;
    }

    uint32_t buildVerInfo::vshBuild()
    {
        return _vsh_build;
    }

    uint32_t buildVerInfo::sdkInternalBuild()
    {
        return _sdk_internal_build;
    }



}; // coredump
}; // vita

