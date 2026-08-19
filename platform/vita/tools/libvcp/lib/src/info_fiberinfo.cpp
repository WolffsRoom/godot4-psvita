#include "info_fiberinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct fiberInfo_info {
    uint32_t size; // zeroes
    uint32_t addr; // 04
    char name[32]; // 08
    uint32_t fiber_state; // 28
    uint32_t context_addr; // 2C
    uint32_t context_size; // 30
    uint32_t entry; // 34
    uint32_t thread_id; // 38
    uint32_t FPSCR; // 38
    uint32_t GPRs[10]; // 3C
    uint64_t neon[8]; // 64
};

struct fiberInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of fiberInfo_info follows
};

*/

    fiberInfo::fiberInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _fibers.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_fibers[i]._address);
            read_string(_fibers[i]._name, 32);
            read_var(_fibers[i]._state);
            read_var(_fibers[i]._context_address);
            read_var(_fibers[i]._context_size);
            read_var(_fibers[i]._entry);
            read_var(_fibers[i]._thread_id);
            read_var(_fibers[i]._FPSCR);
            read_array(_fibers[i]._GPRs, 10*4);
            read_array(_fibers[i]._neon, 8*8);
        }
    }

    std::vector<fiberInfoFiber>& fiberInfo::fibers()
    {
        return _fibers;
    }

    uint32_t fiberInfoFiber::address()
    {
        return _address;
    }

    std::string& fiberInfoFiber::name()
    {
        return _name;
    }

    uint32_t fiberInfoFiber::state()
    {
        return _state;
    }

    std::string fiberInfoFiber::readableState()
    {
        std::vector<std::string> state;

        switch ((fiberState)_state)
        {
            case fiberState::FIBER_STATE_INITALIZED:
                state.push_back("FIBER_STATE_INITALIZED");
                break;
            case fiberState::FIBER_STATE_RUNNING:
                state.push_back("FIBER_STATE_RUNNING");
                break;
            case fiberState::FIBER_STATE_SUSPENDED:
                state.push_back("FIBER_STATE_SUSPENDED");
                break;
        }
        return utils::join(state, " | ");

    }

    uint32_t fiberInfoFiber::contextAddress()
    {
        return _context_address;
    }

    uint32_t fiberInfoFiber::contextSize()
    {
        return _context_size;
    }

    uint32_t fiberInfoFiber::entry()
    {
        return _entry;
    }

    uint32_t fiberInfoFiber::threadId()
    {
        return _thread_id;
    }

    uint32_t fiberInfoFiber::FPSCR()
    {
        return _FPSCR;
    }

    std::array<uint32_t,10>& fiberInfoFiber::GPRs()
    {
        return _GPRs;
    }

    std::array<uint64_t,8>& fiberInfoFiber::neon()
    {
        return _neon;
    }


}; // coredump
}; // vita

