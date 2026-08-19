#include "info_threadinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct threadInfo_thread {
    uint32_t size; // size
    uint32_t thread_id; // 04
    char name[32]; // 08
    uint32_t initial_attributes; // 28
    uint32_t attributes; // 2C
    uint32_t status; // 30 check

    uint32_t entry_address; // 34
    uint32_t stack_address_top; // 38
    uint32_t stack_size; // 3C

    uint32_t stack_used_size; // 40, zeroes

    uint32_t arg_size; // 44
    uint32_t arg_block_base_addr; // 48

    uint32_t initial_priority; // 4C
    uint32_t priority; // 50
    uint32_t initial_affinity_mask; // 54
    uint32_t affinity_mask; // 58
    uint32_t last_cpu_id; // 5C
    uint32_t wait_state_type; // 60
    uint32_t wait_target_id; // 64
    uint64_t clock_run; // 68

    uint32_t unk70; // 70, zeroes
    uint32_t stop_reason; // 74
    uint32_t unk78; // 78, zeroes
    uint32_t unk7C; // 7C, zeroes
    uint32_t unk80; // 80, zeroes

    uint32_t exit_status; // 84
    uint32_t preempted_by_int_count; // 88
    uint32_t preempted_by_thread_count; // 8C
    uint32_t voluntarily_release_count; // 90
    uint32_t change_cpu_count; // 94
    uint32_t vfp_mode; // 98
    uint32_t PC; // 9C
    uint32_t wait_timeout; // A0

    // depends on wait_state_type, see waitType
    // looks like maximum 3 uint32_t used
    uint32_t unkA4; // A4
    uint32_t unkA8; // A8
    uint32_t unkAC; // AC

    // everything below is zeroed
    uint32_t unkB0; // B0
    uint32_t unkB4; // B4
    uint32_t unkB8; // B8
    uint32_t unkBC; // BC
    uint32_t unkC0; // C0
    uint32_t unkC4; // C4
};


struct threadInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of threadInfo_thread follows
};
*/

    threadInfo::threadInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _threads.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_threads[i]._id);
            read_string(_threads[i]._name, 32);
            read_var(_threads[i]._initial_attributes);
            read_var(_threads[i]._attributes);
            read_var(_threads[i]._status);
            read_var(_threads[i]._entry);
            read_var(_threads[i]._stack_top_address);
            read_var(_threads[i]._stack_size);
            read_var(_threads[i]._stack_used_size);
            read_var(_threads[i]._arg_size);
            read_var(_threads[i]._arg_block_address);
            read_var(_threads[i]._initial_priority);
            read_var(_threads[i]._priority);
            read_var(_threads[i]._initial_affinity);
            read_var(_threads[i]._affinity);
            read_var(_threads[i]._last_cpu_id);
            read_var(_threads[i]._wait_state_type);
            read_var(_threads[i]._wait_target_id);
            read_var(_threads[i]._clock_run);

            in.seekg(4, std::ios_base::cur);
            read_var(_threads[i]._stop_reason);
            in.seekg(4, std::ios_base::cur);
            in.seekg(4, std::ios_base::cur);
            in.seekg(4, std::ios_base::cur);

            read_var(_threads[i]._exit_status);

            read_var(_threads[i]._preempted_by_int_count);
            read_var(_threads[i]._preempted_by_thread_count);
            read_var(_threads[i]._voluntarily_release_count);
            read_var(_threads[i]._change_cpu_count);

            read_var(_threads[i]._vfp_mode);
            read_var(_threads[i]._PC);
            read_var(_threads[i]._wait_timeout);

            switch ((waitType)_threads[i]._wait_state_type)
            {
                case waitType::WAITTYPE_DELAY:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailDelay{};
                        auto d = std::get_if<threadInfoWaitDetailDelay>(&_threads[i]._wait_data);
                        read_var(d->delay);
                        in.seekg(4*7, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_SIGNAL:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailSignal{};
                        auto d = std::get_if<threadInfoWaitDetailSignal>(&_threads[i]._wait_data);
                        read_var(d->option);
                        in.seekg(4*8, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_EVENTFLAG:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailEventFlag{};
                        auto d = std::get_if<threadInfoWaitDetailEventFlag>(&_threads[i]._wait_data);
                        read_var(d->bit_pattern);
                        read_var(d->wait_mode);
                        in.seekg(4*7, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_SEMAPHORE:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailSemaphore{};
                        auto d = std::get_if<threadInfoWaitDetailSemaphore>(&_threads[i]._wait_data);
                        read_var(d->need_count);
                        in.seekg(4*8, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_MUTEX:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailMutex{};
                        auto d = std::get_if<threadInfoWaitDetailMutex>(&_threads[i]._wait_data);
                        read_var(d->condition_id);
                        read_var(d->lock_count);
                        in.seekg(4*7, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_RW_LOCK:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailRWLock{};
                        auto d = std::get_if<threadInfoWaitDetailRWLock>(&_threads[i]._wait_data);
                        read_var(d->mode);
                        in.seekg(4*8, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_COND_MUTEX:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailCondition{};
                        auto d = std::get_if<threadInfoWaitDetailCondition>(&_threads[i]._wait_data);
                        read_var(d->mutex_id);
                        read_var(d->condition_id);
                        in.seekg(4*7, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_EVENT:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailEvent{};
                        auto d = std::get_if<threadInfoWaitDetailEvent>(&_threads[i]._wait_data);
                        read_var(d->wait_pattern);
                        in.seekg(4*8, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_MP_EVENTS:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailMultipleEvents{};
                        auto d = std::get_if<threadInfoWaitDetailMultipleEvents>(&_threads[i]._wait_data);
                        read_var(d->wait_event_list_address);
                        read_var(d->number_of_events);
                        read_var(d->wait_mode);
                        in.seekg(4*6, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_MSG_PIPE:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailMessagePipe{};
                        auto d = std::get_if<threadInfoWaitDetailMessagePipe>(&_threads[i]._wait_data);
                        read_var(d->original_size);
                        read_var(d->remaining_size);
                        read_var(d->wait_mode);
                        in.seekg(4*6, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_LW_MUTEX:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailLWMutex{};
                        auto d = std::get_if<threadInfoWaitDetailLWMutex>(&_threads[i]._wait_data);
                        read_var(d->lw_condition_id);
                        read_var(d->lock_count);
                        in.seekg(4*7, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_LW_COND_LW_MUTEX:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailLWCondition{};
                        auto d = std::get_if<threadInfoWaitDetailLWCondition>(&_threads[i]._wait_data);
                        read_var(d->lw_mutex_id);
                        read_var(d->lw_condition_id);
                        in.seekg(4*7, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_EXCEPTION_VM:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailExceptionForVM{};
                        auto d = std::get_if<threadInfoWaitDetailExceptionForVM>(&_threads[i]._wait_data);
                        read_var(d->type_mask);
                        in.seekg(4*8, std::ios_base::cur);
                    }
                    break;
                case waitType::WAITTYPE_NONE:
                        in.seekg(4*9, std::ios_base::cur);
                        break;
                default:
                    {
                        _threads[i]._wait_data = threadInfoWaitDetailUnknown{};
                        auto d = std::get_if<threadInfoWaitDetailUnknown>(&_threads[i]._wait_data);
                        read_var(d->data1);
                        read_var(d->data2);
                        read_var(d->data3);
                        in.seekg(4*6, std::ios_base::cur);
                    }
                    break;
            }

        }
    }

    std::vector<threadInfoThread>& threadInfo::threads()
    {
        return _threads;
    }

    uint32_t threadInfoThread::id()
    {
        return _id;
    }

    std::string& threadInfoThread::name()
    {
        return _name;
    }

    uint32_t threadInfoThread::initialAttributes()
    {
        return _initial_attributes;
    }

    std::string threadInfoThread::readableInitialAttributes()
    {
        // 0x80000000 = user thread
        return std::string("TODO");
    }

    uint32_t threadInfoThread::attributes()
    {
        return _attributes;
    }

    std::string threadInfoThread::readableAttributes()
    {
        return std::string("TODO");
    }

    uint32_t threadInfoThread::status()
    {
        return _status;
    }

    std::string threadInfoThread::readableStatus()
    {
        std::vector<std::string> status;

        if (_status & (uint32_t)threadStatus::THREAD_STATUS_RUNNING)
        {
            status.push_back("THREAD_STATUS_RUNNING");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_READY)
        {
            status.push_back("THREAD_STATUS_READY");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_STANDBY)
        {
            status.push_back("THREAD_STATUS_STANDBY");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_WAITING)
        {
            status.push_back("THREAD_STATUS_WAITING");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_DORMANT)
        {
            status.push_back("THREAD_STATUS_DORMANT");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_DELETED)
        {
            status.push_back("THREAD_STATUS_DELETED");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_DEAD)
        {
            status.push_back("THREAD_STATUS_DEAD");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_STAGNANT)
        {
            status.push_back("THREAD_STATUS_STAGNANT");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_VM_SUSPENDED)
        {
            status.push_back("THREAD_STATUS_VM_SUSPENDED");
        }
        if (_status & (uint32_t)threadStatus::THREAD_STATUS_INSIDE_SYSCALL)
        {
            status.push_back("THREAD_STATUS_INSIDE_SYSCALL");
        }

        return utils::join(status, " | ");
    }

    uint32_t threadInfoThread::entry()
    {
        return _entry;
    }


    uint32_t threadInfoThread::stackTopAddress()
    {
        return _stack_top_address;
    }

    uint32_t threadInfoThread::stackSize()
    {
        return _stack_size;
    }

    uint32_t threadInfoThread::stackUsedSize()
    {
        return _stack_used_size;
    }

    uint32_t threadInfoThread::argSize()
    {
        return _arg_size;
    }

    uint32_t threadInfoThread::argBlockAddress()
    {
        return _arg_block_address;
    }

    uint32_t threadInfoThread::initialPriority()
    {
        return _initial_priority;
    }

    uint32_t threadInfoThread::priority()
    {
        return _priority;
    }

    uint32_t threadInfoThread::initialAffinity()
    {
        return _initial_affinity;
    }

    std::string threadInfoThread::readableInitialAffinity()
    {
        std::vector<std::string> affinity;

        if (_initial_affinity & (1 << 16))
            affinity.push_back("CPU0");
        if (_initial_affinity & (2 << 16))
            affinity.push_back("CPU1");
        if (_initial_affinity & (4 << 16))
            affinity.push_back("CPU2");
        if (_initial_affinity & (8 << 16))
            affinity.push_back("CPU3");

        return utils::join(affinity, " | ");
    }

    uint32_t threadInfoThread::affinity()
    {
        return _affinity;
    }


    std::string threadInfoThread::readableAffinity()
    {
        std::vector<std::string> affinity;

        if (_affinity & (1 << 16))
            affinity.push_back("CPU0");
        if (_affinity & (2 << 16))
            affinity.push_back("CPU1");
        if (_affinity & (4 << 16))
            affinity.push_back("CPU2");
        if (_affinity & (8 << 16))
            affinity.push_back("CPU3");

        return utils::join(affinity, " | ");
    }

    uint32_t threadInfoThread::lastCpuId()
    {
        return _last_cpu_id;
    }

    uint32_t threadInfoThread::waitStateType()
    {
        return _wait_state_type;
    }

    std::string threadInfoThread::readableWaitStateType()
    {
        switch((waitType)_wait_state_type) {
            case waitType::WAITTYPE_NONE:
                return std::string("WAITTYPE_NONE");
            case waitType::WAITTYPE_DELAY:
                return std::string("WAITTYPE_DELAY");
            case waitType::WAITTYPE_WAITTHEND:
                return std::string("WAITTYPE_WAITTHEND");
            case waitType::WAITTYPE_SIGNAL:
                return std::string("WAITTYPE_SIGNAL");
            case waitType::WAITTYPE_WAITTHSUSPEND:
                return std::string("WAITTYPE_WAITTHSUSPEND");
            case waitType::WAITTYPE_EVENTFLAG:
                return std::string("WAITTYPE_EVENTFLAG");
            case waitType::WAITTYPE_SEMAPHORE:
                return std::string("WAITTYPE_SEMAPHORE");
            case waitType::WAITTYPE_MUTEX:
                return std::string("WAITTYPE_MUTEX");
            case waitType::WAITTYPE_RW_LOCK:
                return std::string("WAITTYPE_RW_LOCK");
            case waitType::WAITTYPE_COND_SIGNAL:
                return std::string("WAITTYPE_COND_SIGNAL");
            case waitType::WAITTYPE_COND_MUTEX:
                return std::string("WAITTYPE_COND_MUTEX");
            case waitType::WAITTYPE_FAST_MUTEX:
                return std::string("WAITTYPE_FAST_MUTEX");
            case waitType::WAITTYPE_FAST_MUTEX_SPIN:
                return std::string("WAITTYPE_FAST_MUTEX_SPIN");
            case waitType::WAITTYPE_EVENT:
                return std::string("WAITTYPE_EVENT");
            case waitType::WAITTYPE_MP_EVENTS:
                return std::string("WAITTYPE_MP_EVENTS");
            case waitType::WAITTYPE_MSG_PIPE:
                return std::string("WAITTYPE_MSG_PIPE");
            case waitType::WAITTYPE_LW_MUTEX:
                return std::string("WAITTYPE_LW_MUTEX");
            case waitType::WAITTYPE_LW_COND_SIGNAL:
                return std::string("WAITTYPE_LW_COND_SIGNAL");
            case waitType::WAITTYPE_LW_COND_LW_MUTEX:
                return std::string("WAITTYPE_LW_COND_LW_MUTEX");
            case waitType::WAITTYPE_EXCEPTION_VM:
                return std::string("WAITTYPE_EXCEPTION_VM");
        }
        return std::string("WAITTYPE_UNKNOWN");
    }


    uint32_t threadInfoThread::waitTargetId()
    {
        return _wait_target_id;
    }

    uint64_t threadInfoThread::clockRun()
    {
        return _clock_run;
    }

    uint32_t threadInfoThread::stopReason()
    {
        return _stop_reason;
    }

    std::string threadInfoThread::readableStopReason()
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

    uint32_t threadInfoThread::exitStatus()
    {
        return _exit_status;
    }

    uint32_t threadInfoThread::preemptedByIntCount()
    {
        return _preempted_by_int_count;
    }

    uint32_t threadInfoThread::preemptedByThreadCount()
    {
        return _preempted_by_thread_count;
    }

    uint32_t threadInfoThread::voluntarilyReleaseCount()
    {
        return _voluntarily_release_count;
    }

    uint32_t threadInfoThread::changeCpuCount()
    {
        return _change_cpu_count;
    }

    uint32_t threadInfoThread::vfpMode()
    {
        return _vfp_mode;
    }

    uint32_t threadInfoThread::PC()
    {
        return _PC;
    }

    uint32_t threadInfoThread::waitTimeout()
    {
        return _wait_timeout;
    }

    std::variant<
        threadInfoWaitDetailCondition,
        threadInfoWaitDetailDelay,
        threadInfoWaitDetailEvent,
        threadInfoWaitDetailEventFlag,
        threadInfoWaitDetailExceptionForVM,
        threadInfoWaitDetailLWCondition,
        threadInfoWaitDetailLWMutex,
        threadInfoWaitDetailMessagePipe,
        threadInfoWaitDetailMutex,
        threadInfoWaitDetailRWLock,
        threadInfoWaitDetailSemaphore,
        threadInfoWaitDetailSignal,
        threadInfoWaitDetailMultipleEvents,
        threadInfoWaitDetailUnknown
    >& threadInfoThread::waitData()
    {
        return _wait_data;
    }

}; // coredump
}; // vita

