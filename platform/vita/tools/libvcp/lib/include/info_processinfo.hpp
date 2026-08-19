#ifndef VITA_COREDUMP_PROCESSINFO_H
#define VITA_COREDUMP_PROCESSINFO_H

#include <cstdint>
#include <string>
#include "defines.hpp"

namespace vita
{
namespace coredump
{




class processInfo {
    public:
        processInfo(void* buf, uint32_t size);

        int32_t pid();
        uint32_t processAttr();
        std::string readableProcessAttr();

        std::string& titleId();
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

        uint64_t time();

    private:
        int32_t _pid;
        uint32_t _process_attr; // PROCESS_ATTR_* // 8
        std::string _title_id;
        uint32_t _cpu_affinity;
        uint32_t _start_entry_addr;
        uint32_t _fingerprint;
        int32_t _parent_pid;

        uint32_t _stop_reason;

        std::string _path;

        uint32_t _arm_exidx_start;
        uint32_t _arm_exidx_end;
        uint32_t _arm_extab_start;
        uint32_t _arm_extab_end;

        uint64_t _time; // process time in microseconds
}; // processInfo

}; // coredump
}; // vita

#endif