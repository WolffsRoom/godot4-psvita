#include "info_screenshotinfo.hpp"

#include <cstdint>
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct screenshotInfo_struct {
      uint32_t unk1; // min. coredump version
      uint32_t unk2;

      uint32_t unk3; // maybe vsync count?
      uint32_t width;
      uint32_t height;
      uint32_t unk4;
      // then goes raw RGBA framebuf as uint32_t data[width*height]
    };
*/


    screenshotInfo::screenshotInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(12, std::ios_base::beg);
        read_var(_width);
        read_var(_height);
        in.seekg(4, std::ios_base::cur);
        read_vector(_data, _width*_height*4);
    }

    std::vector<uint8_t>& screenshotInfo::data()
    {
        return _data;
    }

    uint32_t screenshotInfo::width()
    {
        return _width;
    }

    uint32_t screenshotInfo::height()
    {
        return _height;
    }

}; // coredump
}; // vita

