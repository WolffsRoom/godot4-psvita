#include "info_ttyinfo.hpp"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct ttyInfo_struct { // and ttyInfo2
      uint32_t unk1; // min. coredump version
      uint32_t unk2;

      uint32_t length; // seems to be always 4096

      char buf[4096];
    };
*/

    ttyInfo::ttyInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(8, std::ios_base::beg);
        read_var(_length);
        read_string(_data, _length);
    }

    std::string& ttyInfo::data()
    {
        return _data;
    }

    uint32_t ttyInfo::length()
    {
        return _length;
    }

}; // coredump
}; // vita

