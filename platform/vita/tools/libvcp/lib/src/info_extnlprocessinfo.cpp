#include "info_extnlprocessinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{

/*
struct extnlProcessInfo_proc {
    uint32_t zeroes; // 0x00
    int32_t pid; // 0x04
    uint32_t budget; // 0x08
    uint32_t process_attr; // 0x0C
    char name[32]; // 0x10
    uint32_t unk_2C; // 0x30
    uint32_t cpu_affinity; // 34
    uint32_t start_entry_addr; // 38
    uint32_t fingerprint; // 3C
    int32_t parent_pid; // 40
    uint32_t unk44; // zeroes
    uint32_t stop_reason; // 48
    uint32_t unk4C; // zeroes
    uint32_t unk50; // zeroes
    uint32_t unk54; // zeroes
    uint32_t path_length; // zeroes

    // char path[path_length rounded towards 4], so 25->28, etc?
    // uint32_t arm_exidx_start;
    // uint32_t arm_exidx_end;
    // uint32_t arm_extab_start;
    // uint32_t arm_extab_end;
};


struct extnlProcessInfo {
    uint32_t unk0; // min. coredump version
    uint32_t count;
    // count extnlProcessInfo_proc structs follow
};
*/
    extnlProcessInfo::extnlProcessInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _procs.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);
            read_var(_procs[i]._pid);
            read_var(_procs[i]._budget);
            read_var(_procs[i]._process_attr);
            read_string(_procs[i]._name, 32);
            in.seekg(4, std::ios_base::cur);
            read_var(_procs[i]._cpu_affinity);
            read_var(_procs[i]._start_entry_addr);
            read_var(_procs[i]._fingerprint);
            read_var(_procs[i]._parent_pid);
            in.seekg(4, std::ios_base::cur);
            read_var(_procs[i]._stop_reason);
            in.seekg(12, std::ios_base::cur);

            uint32_t path_length;

            read_var(path_length);
            uint32_t path_size = (path_length + 3) & ~0x03; // align to 4

            read_string(_procs[i]._path, path_size);

            read_var(_procs[i]._arm_exidx_start);
            read_var(_procs[i]._arm_exidx_end);
            read_var(_procs[i]._arm_extab_start);
            read_var(_procs[i]._arm_extab_end);
        }
    }

    std::vector<extnlProcessInfoProc>& extnlProcessInfo::procs()
    {
        return _procs;
    }

    int32_t extnlProcessInfoProc::pid()
    {
        return _pid;
    }

    uint32_t extnlProcessInfoProc::budget()
    {
        return _budget;
    }

    uint32_t extnlProcessInfoProc::processAttr()
    {
        return _process_attr;
    }

    std::string extnlProcessInfoProc::readableProcessAttr()
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

    std::string& extnlProcessInfoProc::name()
    {
        return _name;
    }

    uint32_t extnlProcessInfoProc::cpuAffinity()
    {
        return _cpu_affinity;
    }

    std::string extnlProcessInfoProc::readableCpuAffinity()
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

    uint32_t extnlProcessInfoProc::entryAddr()
    {
        return _start_entry_addr;
    }

    uint32_t extnlProcessInfoProc::fingerprint()
    {
        return _fingerprint;
    }

    int32_t extnlProcessInfoProc::parentPid()
    {
        return _parent_pid;
    }

    uint32_t extnlProcessInfoProc::stopReason()
    {
        return _stop_reason;
    }

    std::string extnlProcessInfoProc::readableStopReason()
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

    std::string& extnlProcessInfoProc::path()
    {
        return _path;
    }


    uint32_t extnlProcessInfoProc::armExidxStart()
    {
        return _arm_exidx_start;
    }

    uint32_t extnlProcessInfoProc::armExidxEnd()
    {
        return _arm_exidx_end;
    }

    uint32_t extnlProcessInfoProc::armExtabStart()
    {
        return _arm_extab_start;
    }

    uint32_t extnlProcessInfoProc::armExtabEnd()
    {
        return _arm_extab_end;
    }


}; // coredump
}; // vita

