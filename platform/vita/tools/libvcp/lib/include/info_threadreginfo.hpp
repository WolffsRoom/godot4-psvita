#ifndef VITA_COREDUMP_THREADREGINFO_H
#define VITA_COREDUMP_THREADREGINFO_H

#include <cstdint>
#include <string>
#include <array>
#include <vector>

namespace vita
{
namespace coredump
{

class threadRegInfo;

class threadRegInfoThreadReg {
    friend threadRegInfo;
    public:
        uint32_t threadId();

        std::array<uint32_t, 16>& registers();
        uint32_t CPSR();

        uint32_t FPSCR();
        uint32_t TPIDRURO();
        uint32_t TPIDRURW();

        std::array<uint64_t, 32>& neon();

        uint32_t FPEXC();
        uint32_t CPACR();
        uint32_t DACR();

        uint32_t DBGDSCR();
        uint32_t IFSR();
        uint32_t IFAR();
        uint32_t DFSR();
        uint32_t DFAR();

    private:
        uint32_t _thread_id;
        std::array<uint32_t, 16> _registers;
        uint32_t _CPSR;

        uint32_t _FPSCR;
        uint32_t _TPIDRURO;
        uint32_t _TPIDRURW;

        std::array<uint64_t, 32> _neon;

        uint32_t _FPEXC;
        uint32_t _CPACR;
        uint32_t _DACR;

        uint32_t _DBGDSCR;
        uint32_t _IFSR;
        uint32_t _IFAR;
        uint32_t _DFSR;
        uint32_t _DFAR;
}; // threadRegInfoThreadReg


class threadRegInfo {
    public:
        threadRegInfo(void* buf, uint32_t size);
        std::vector<threadRegInfoThreadReg>& threadRegs();

    private:
        std::vector<threadRegInfoThreadReg> _thread_regs;
}; // threadRegInfo

}; // coredump
}; // vita

#endif