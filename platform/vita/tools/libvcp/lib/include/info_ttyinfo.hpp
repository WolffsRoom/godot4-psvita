#ifndef VITA_COREDUMP_TTYINFO_H
#define VITA_COREDUMP_TTYINFO_H

#include <cstdint>
#include <string>

namespace vita
{
namespace coredump
{

class ttyInfo {
    public:
        ttyInfo(void* buf, uint32_t size);

        uint32_t length();
        std::string& data();

    private:
        uint32_t _length;
        std::string _data;
}; // ttyInfo

}; // coredump
}; // vita

#endif