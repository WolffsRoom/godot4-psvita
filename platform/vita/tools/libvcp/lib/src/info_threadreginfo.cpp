#include "info_threadreginfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct threadRegInfo_info { // 0x178 size
    uint32_t size; // this struct size. can be 0
    uint32_t thread_id; // 04
    uint32_t r0; // 08
    uint32_t r1; // 0C
    uint32_t r2; // 10
    uint32_t r3; // 14
    uint32_t r4; // 18
    uint32_t r5; // 1C
    uint32_t r6; // 20
    uint32_t r7; // 24
    uint32_t r8; // 28
    uint32_t r9; // 2C
    uint32_t r10; // 30
    uint32_t r11; // 34
    uint32_t r12; // 38
    uint32_t r13; // 3C
    uint32_t r14; // 40
    uint32_t r15; // 44
    uint32_t CPSR; // 48

    uint32_t FPSCR; // 4C
    uint32_t TPIDRURO; // 50

    uint32_t neon0[4]; // 54
    uint32_t neon1[4]; // 64
    uint32_t neon2[4]; // 74
    uint32_t neon3[4]; // 84
    uint32_t neon4[4]; // 94
    uint32_t neon5[4]; // A4
    uint32_t neon6[4]; // B4
    uint32_t neon7[4]; // C4
    uint32_t neon8[4]; // D4
    uint32_t neon9[4]; // E4
    uint32_t neon10[4]; // F4
    uint32_t neon11[4]; // 104
    uint32_t neon12[4]; // 114
    uint32_t neon13[4]; // 124
    uint32_t neon14[4]; // 134
    uint32_t neon15[4]; // 144
    uint32_t FPEXC; // 154
    uint32_t TPIDRURW; // 158

    uint32_t CPACR; // 15C
    uint32_t DACR; // 160

    uint32_t DBGDSCR; // 164
    uint32_t IFSR; // 168
    uint32_t IFAR; // 16C
    uint32_t DFSR; // 170
    uint32_t DFAR; // 174

};

struct threadRegInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of threadRegInfo_info follows
};

*/

    threadRegInfo::threadRegInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _thread_regs.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_thread_regs[i]._thread_id);
            read_array(_thread_regs[i]._registers, 16*4);
            read_var(_thread_regs[i]._CPSR);
            read_var(_thread_regs[i]._FPSCR);
            read_var(_thread_regs[i]._TPIDRURO);
            read_array(_thread_regs[i]._neon, 32*8);
            read_var(_thread_regs[i]._FPEXC);
            read_var(_thread_regs[i]._TPIDRURW);
            read_var(_thread_regs[i]._CPACR);
            read_var(_thread_regs[i]._DACR);
            read_var(_thread_regs[i]._DBGDSCR);
            read_var(_thread_regs[i]._IFSR);
            read_var(_thread_regs[i]._IFAR);
            read_var(_thread_regs[i]._DFSR);
            read_var(_thread_regs[i]._DFAR);
        }
    }

    std::vector<threadRegInfoThreadReg>& threadRegInfo::threadRegs()
    {
        return _thread_regs;
    }

    uint32_t threadRegInfoThreadReg::threadId()
    {
        return _thread_id;
    }

    std::array<uint32_t, 16>& threadRegInfoThreadReg::registers()
    {
        return _registers;
    }

    uint32_t threadRegInfoThreadReg::CPSR()
    {
        return _CPSR;
    }

    uint32_t threadRegInfoThreadReg::FPSCR()
    {
        return _FPSCR;
    }

    uint32_t threadRegInfoThreadReg::TPIDRURO()
    {
        return _TPIDRURO;
    }

    uint32_t threadRegInfoThreadReg::TPIDRURW()
    {
        return _TPIDRURW;
    }

    std::array<uint64_t, 32>& threadRegInfoThreadReg::neon()
    {
        return _neon;
    }

    uint32_t threadRegInfoThreadReg::FPEXC()
    {
        return _FPEXC;
    }

    uint32_t threadRegInfoThreadReg::CPACR()
    {
        return _CPACR;
    }

    uint32_t threadRegInfoThreadReg::DACR()
    {
        return _DACR;
    }

    uint32_t threadRegInfoThreadReg::DBGDSCR()
    {
        return _DBGDSCR;
    }

    uint32_t threadRegInfoThreadReg::IFSR()
    {
        return _IFSR;
    }

    uint32_t threadRegInfoThreadReg::IFAR()
    {
        return _IFAR;
    }

    uint32_t threadRegInfoThreadReg::DFSR()
    {
        return _DFSR;
    }

    uint32_t threadRegInfoThreadReg::DFAR()
    {
        return _DFAR;
    }

}; // coredump
}; // vita

