#ifndef VITA_COREDUMP_COREINFO_H
#define VITA_COREDUMP_COREINFO_H

#include <cstdint>

namespace vita
{
namespace coredump
{

class corefileInfo {
    public:
        corefileInfo(void* buf, uint32_t size);

        uint32_t version();
        bool isInternal();

    private:
        uint32_t _version;
        bool _is_internal;
}; // corefileInfo

}; // coredump
}; // vita

#endif