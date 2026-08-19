#ifndef VITA_COREDUMP_SCREENSHOTINFO_H
#define VITA_COREDUMP_SCREENSHOTINFO_H

#include <cstdint>
#include <vector>

namespace vita
{
namespace coredump
{

class screenshotInfo {
    public:
        screenshotInfo(void* buf, uint32_t size);

        uint32_t width();
        uint32_t height();
        std::vector<uint8_t>& data();

    private:
        uint32_t _width;
        uint32_t _height;
        std::vector<uint8_t> _data;
}; // screenshotInfo

}; // coredump
}; // vita

#endif