#include "info_corefileinfo.hpp"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    typedef struct  { // 0x4 + 0x24
      uint32_t unk1; // min. coredump version

      uint32_t unk2; // 0x00

      uint32_t version; // 0x13 on 3.60-3.73, 0x10 on 3.10, 0x9 on 2.60, 0x1 on 0.945 | 0x04
      uint32_t internal; // 1 - third-party, 2 - system/internal/webkit (or henkaku) | 0x08
      uint32_t unk3[6]; // always zero? encoding or encrypt? sets only first uint32 | 0x0C
    } corefileInfo_struct;
*/

    corefileInfo::corefileInfo(void* buf, uint32_t size) : _version(0)
    {
        memstream in((const char*)buf, size);

        uint32_t internal;

        in.seekg(8, std::ios_base::beg);

        read_var(_version);
        read_var(internal);

        _is_internal = internal - 1;
    }

    bool corefileInfo::isInternal()
    {
        return _is_internal;
    }

    uint32_t corefileInfo::version()
    {
        return _version;
    }

}; // coredump
}; // vita

