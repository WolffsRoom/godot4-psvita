#ifndef VITA_COREDUMP_EXTNLPROCESSINFO_H
#define VITA_COREDUMP_EXTNLPROCESSINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "defines.hpp"

namespace vita
{
namespace coredump
{

class extnlProcessInfo;

class extnlProcessInfoProc {
    friend extnlProcessInfo;
    public:
        int32_t pid();

        uint32_t budget();

        uint32_t processAttr();
        std::string readableProcessAttr();

        std::string& name();

        uint32_t cpuAffinity();
        std::string readableCpuAffinity();

        uint32_t entryAddr();
        uint32_t fingerprint();
        int32_t parentPid();

        uint32_t stopReason();
        std::string readableStopReason();

        std::string& path();

        uint32_t armExidxStart();
        uint32_t armExidxEnd();
        uint32_t armExtabStart();
        uint32_t armExtabEnd();

    private:
        uint32_t _pid;
        uint32_t _budget;
        uint32_t _process_attr;
        std::string _name; // or titleId
        uint32_t _cpu_affinity;
        uint32_t _start_entry_addr;
        uint32_t _fingerprint;
        uint32_t _parent_pid;
        uint32_t _stop_reason;
        std::string _path;
        uint32_t _arm_exidx_start;
        uint32_t _arm_exidx_end;
        uint32_t _arm_extab_start;
        uint32_t _arm_extab_end;
}; // extnlProcessInfoProc


class extnlProcessInfo {
    public:
        extnlProcessInfo(void* buf, uint32_t size);
        std::vector<extnlProcessInfoProc>& procs();

    private:
        std::vector<extnlProcessInfoProc> _procs;
}; // extnlProcessInfo

}; // coredump
}; // vita

#endif