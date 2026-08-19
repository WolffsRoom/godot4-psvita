#include "info_processinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct processInfo_struct { // 0x4 + 0x90
        uint32_t unk0; // min. coredump version
        uint32_t unk4; // zeroes // 0

        int32_t pid; // 4
        uint32_t process_attr; // PROCESS_ATTR_* // 8
        char title_id[32]; // contains titleid in first 10 bytes // 0xC
        uint32_t unk2C; // seems to be always zero // 2C
        uint32_t cpu_affinity; // 30
        uint32_t start_entry_addr; // 34
        uint32_t fingerprint; // 38
        int32_t parent_pid; // 3C

        uint32_t unk40; // zeroes
        uint32_t stop_reason; // 44
        uint32_t additional_id; // 48 // not set - zeroes
        uint32_t unk4C; // zeroes
        uint32_t unk50; // zeroes

        uint32_t path_length; // 54
        // char path[path_length rounded towards 4], so 25->28, etc? // 58

        // uint32_t arm_exidx_start;
        // uint32_t arm_exidx_end;
        // uint32_t arm_extab_start;
        // uint32_t arm_extab_end;

        // uint64_t time; // process time in microseconds
    };
    */



    processInfo::processInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(8, std::ios_base::beg);

        read_var(_pid);
        read_var(_process_attr);
        read_string(_title_id, 32);
        in.seekg(4, std::ios_base::cur);
        read_var(_cpu_affinity);
        read_var(_start_entry_addr);
        read_var(_fingerprint);
        read_var(_parent_pid);
        in.seekg(4, std::ios_base::cur);
        read_var(_stop_reason);
        in.seekg(12, std::ios_base::cur);

        uint32_t path_length;

        read_var(path_length);
        uint32_t path_size = (path_length + 3) & ~0x03; // align to 4

        read_string(_path, path_size);

        read_var(_arm_exidx_start);
        read_var(_arm_exidx_end);
        read_var(_arm_extab_start);
        read_var(_arm_extab_end);

        read_var(_time);
    }


    int32_t processInfo::pid()
    {
        return _pid;
    }

    uint32_t processInfo::processAttr()
    {
        return _process_attr;
    }

    std::string processInfo::readableProcessAttr()
    {
        std::vector<std::string> attr;
        if (_process_attr & (uint32_t)processAttr::PROCESS_ATTR_LOADED)
            attr.push_back("PROCESS_ATTR_LOADED");

        if (_process_attr & (uint32_t)processAttr::PROCESS_ATTR_DEBUG_STATE)
            attr.push_back("PROCESS_ATTR_DEBUG_STATE");

        if (_process_attr & (uint32_t)processAttr::PROCESS_ATTR_SYSTEM)
            attr.push_back("PROCESS_ATTR_SYSTEM");

        if (_process_attr & (uint32_t)processAttr::PROCESS_ATTR_SUSPENDED)
            attr.push_back("PROCESS_ATTR_SUSPENDED");

        if (_process_attr & (uint32_t)processAttr::PROCESS_ATTR_DEBUGGABLE)
            attr.push_back("PROCESS_ATTR_DEBUGGABLE");

        if (_process_attr & (uint32_t)processAttr::PROCESS_ATTR_ATTACHED)
            attr.push_back("PROCESS_ATTR_ATTACHED");

        if (_process_attr & (uint32_t)processAttr::PROCESS_ATTR_FIBER_STATE)
            attr.push_back("PROCESS_ATTR_FIBER_STATE");

        return utils::join(attr, " | ");

    }

    std::string& processInfo::titleId()
    {
        return _title_id;
    }

    uint32_t processInfo::cpuAffinity()
    {
        return _cpu_affinity;
    }

    std::string processInfo::readableCpuAffinity()
    {
        std::vector<std::string> affinity;

        if (_cpu_affinity & 1)
            affinity.push_back("CPU0");
        if (_cpu_affinity & 2)
            affinity.push_back("CPU1");
        if (_cpu_affinity & 4)
            affinity.push_back("CPU2");
        if (_cpu_affinity & 8)
            affinity.push_back("CPU3");

        return utils::join(affinity, " | ");
    }

    uint32_t processInfo::entryAddr()
    {
        return _start_entry_addr;
    }

    uint32_t processInfo::fingerprint()
    {
        return _fingerprint;
    }

    int32_t processInfo::parentPid()
    {
        return _parent_pid;
    }

    uint32_t processInfo::stopReason()
    {
        return _stop_reason;
    }

    std::string processInfo::readableStopReason()
    {
        if (static_cast<enum stopReason>(_stop_reason) >= stopReason::UNRECOVERABLE_ERROR_LOW
            && static_cast<enum stopReason>(_stop_reason) <= stopReason::UNRECOVERABLE_ERROR_HIGH)
        {
            return std::string("UNRECOVERABLE_ERROR");
        }
        switch (static_cast<enum stopReason>(_stop_reason))
        {
            case stopReason::NOTHING: return std::string("NOTHING");
            case stopReason::SUSPEND_PROCESS: return std::string("SUSPEND_PROCESS");
            case stopReason::SUSPEND_THREAD: return std::string("SUSPEND_THREAD");
            case stopReason::APPLICATION_IS_SUSPENDED: return std::string("APPLICATION_IS_SUSPENDED");
            case stopReason::APPMGR_DETECTED_HUNGUP: return std::string("APPMGR_DETECTED_HUNGUP");
            case stopReason::SPONTANEOUS_EXIT: return std::string("SPONTANEOUS_EXIT");
            case stopReason::STACK_OVERFLOW: return std::string("STACK_OVERFLOW");
            case stopReason::SYSCALL_ERROR_ILLEGAL_CONTEXT: return std::string("SYSCALL_ERROR_ILLEGAL_CONTEXT");
            case stopReason::SYSCALL_ERROR_CRITICAL_USAGE: return std::string("SYSCALL_ERROR_CRITICAL_USAGE");
            case stopReason::SYSCALL_ERROR_ILLEGAL_NUMBER: return std::string("SYSCALL_ERROR_ILLEGAL_NUMBER");
            case stopReason::HARDWARE_WATCHPOINT: return std::string("HARDWARE_WATCHPOINT");
            case stopReason::SOFTWARE_WATCHPOINT: return std::string("SOFTWARE_WATCHPOINT");
            case stopReason::HARDWARE_BRKPT: return std::string("HARDWARE_BRKPT");
            case stopReason::SOFTWARE_BRKPT: return std::string("SOFTWARE_BRKPT");
            case stopReason::STARTUP_FAILED: return std::string("STARTUP_FAILED");
            case stopReason::PRX_STOP_INIT: return std::string("PRX_STOP_INIT");
            case stopReason::DTRACE_BRKPT: return std::string("DTRACE_BRKPT");
            case stopReason::EXCEPTION_UNDEF: return std::string("EXCEPTION_UNDEF");
            case stopReason::EXCEPTION_PREFETCH_ABORT: return std::string("EXCEPTION_PREFETCH_ABORT");
            case stopReason::EXCEPTION_DATA_ABORT: return std::string("EXCEPTION_DATA_ABORT");
            case stopReason::FPU_VFP: return std::string("FPU_VFP");
            case stopReason::FPU_NEON: return std::string("FPU_NEON");
            case stopReason::INT_DIV0: return std::string("INT_DIV0");
            case stopReason::GPU_EXCEPTION: return std::string("GPU_EXCEPTION");
            default: return std::string("UNKNOWN");
        }
        return std::string("UNKNOWN");
    }

    std::string& processInfo::path()
    {
        return _path;
    }


    uint32_t processInfo::armExidxStart()
    {
        return _arm_exidx_start;
    }

    uint32_t processInfo::armExidxEnd()
    {
        return _arm_exidx_end;
    }

    uint32_t processInfo::armExtabStart()
    {
        return _arm_extab_start;
    }

    uint32_t processInfo::armExtabEnd()
    {
        return _arm_extab_end;
    }

    uint64_t processInfo::time()
    {
        return _time;
    }


}; // coredump
}; // vita

