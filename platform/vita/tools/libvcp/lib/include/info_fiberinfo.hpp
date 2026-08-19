#ifndef VITA_COREDUMP_FIBERINFO_H
#define VITA_COREDUMP_FIBERINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include "defines.hpp"

namespace vita
{
namespace coredump
{

class fiberInfo;

class fiberInfoFiber {
    friend fiberInfo;
    public:
        uint32_t address();
        std::string& name();
        uint32_t state();
        std::string readableState();
        uint32_t contextAddress();
        uint32_t contextSize();
        uint32_t entry();
        uint32_t threadId();
        uint32_t FPSCR();
        std::array<uint32_t,10>& GPRs();
        std::array<uint64_t,8>& neon();

    private:
        uint32_t _address;
        std::string _name;
        uint32_t _state;
        uint32_t _context_address;
        uint32_t _context_size;
        uint32_t _entry;
        uint32_t _thread_id;
        uint32_t _FPSCR;
        std::array<uint32_t,10> _GPRs;
        std::array<uint64_t,8> _neon;
}; // fiberInfoFiber


class fiberInfo {
    public:
        fiberInfo(void* buf, uint32_t size);
        std::vector<fiberInfoFiber>& fibers();

    private:
        std::vector<fiberInfoFiber> _fibers;
}; // fiberInfo

}; // coredump
}; // vita

#endif