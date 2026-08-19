#ifndef VITA_COREDUMP_BUILDVERINFO_H
#define VITA_COREDUMP_BUILDVERINFO_H

#include <cstdint>
#include <string>

namespace vita
{
namespace coredump
{


class buildVerInfo {
    public:
        buildVerInfo(void* data, uint32_t size);

        uint32_t systemRevision();
        uint32_t vshRevision();
        uint32_t sysconRevision();

        std::string& systemBranch();
        std::string& vshBranch();
        std::string& sysconBranch();

        uint32_t vshBuild();
        uint32_t sdkInternalBuild();

    private:
        uint32_t _system_revision;
        uint32_t _vsh_revision;
        uint32_t _syscon_revision;

        std::string _system_branch;
        std::string _vsh_branch;
        std::string _syscon_branch;

        uint32_t _sdk_internal_build;
        uint32_t _vsh_build;
}; // buildVerInfo

}; // coredump
}; // vita

#endif