#ifndef VITA_COREDUMP_THREADINFO_H
#define VITA_COREDUMP_THREADINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include "defines.hpp"


namespace vita
{
namespace coredump
{

class threadInfo;

struct threadInfoWaitDetailCondition {
    uint32_t mutex_id;
    uint32_t condition_id;
};

struct threadInfoWaitDetailDelay {
    uint64_t delay;
};

struct threadInfoWaitDetailEvent {
    uint32_t wait_pattern;
};

struct threadInfoWaitDetailEventFlag {
    uint32_t bit_pattern;
    uint32_t wait_mode;
};

struct threadInfoWaitDetailExceptionForVM {
    uint32_t type_mask;
};

struct threadInfoWaitDetailLWCondition {
    uint32_t lw_mutex_id;
    uint32_t lw_condition_id;
};

struct threadInfoWaitDetailLWMutex {
    uint32_t lw_condition_id;
    uint32_t lock_count;
};

struct threadInfoWaitDetailMessagePipe {
    uint32_t original_size;
    uint32_t remaining_size;
    uint32_t wait_mode;
};

struct threadInfoWaitDetailMutex {
    uint32_t condition_id;
    uint32_t lock_count;
};

struct threadInfoWaitDetailRWLock {
    uint32_t mode;
};

struct threadInfoWaitDetailSemaphore {
    uint32_t need_count;
};

struct threadInfoWaitDetailSignal {
    uint32_t option;
};

struct threadInfoWaitDetailMultipleEvents {
    uint32_t wait_event_list_address;
    uint32_t number_of_events;
    uint32_t wait_mode;
};

struct threadInfoWaitDetailUnknown {
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
};

class threadInfoThread {
    friend threadInfo;
    public:
        uint32_t id();
        std::string& name();
        uint32_t initialAttributes();
        std::string readableInitialAttributes();
        uint32_t attributes();
        std::string readableAttributes();

        uint32_t status();
        std::string readableStatus();

        uint32_t entry();

        uint32_t stackTopAddress();
        uint32_t stackSize();
        uint32_t stackUsedSize();

        uint32_t argSize();
        uint32_t argBlockAddress();

        uint32_t initialPriority();
        uint32_t priority();

        uint32_t initialAffinity();
        std::string readableInitialAffinity();
        uint32_t affinity();
        std::string readableAffinity();

        uint32_t lastCpuId();
        uint32_t waitStateType();
        std::string readableWaitStateType();
        uint32_t waitTargetId();
        uint64_t clockRun();

        uint32_t stopReason();
        std::string readableStopReason();

        uint32_t exitStatus();
        uint32_t preemptedByIntCount();
        uint32_t preemptedByThreadCount();
        uint32_t voluntarilyReleaseCount();
        uint32_t changeCpuCount();
        uint32_t vfpMode();
        uint32_t PC();
        uint32_t waitTimeout();
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
        >& waitData();

    private:
        uint32_t _id;
        std::string _name;
        uint32_t _initial_attributes;
        uint32_t _attributes;
        uint32_t _status;

        uint32_t _entry;

        uint32_t _stack_top_address;
        uint32_t _stack_size;
        uint32_t _stack_used_size;

        uint32_t _arg_size;
        uint32_t _arg_block_address;

        uint32_t _initial_priority;
        uint32_t _priority;

        uint32_t _initial_affinity;
        uint32_t _affinity;

        uint32_t _last_cpu_id;
        uint32_t _wait_state_type;
        uint32_t _wait_target_id;
        uint64_t _clock_run;

        uint32_t _stop_reason;

        uint32_t _exit_status;
        uint32_t _preempted_by_int_count;
        uint32_t _preempted_by_thread_count;
        uint32_t _voluntarily_release_count;
        uint32_t _change_cpu_count;
        uint32_t _vfp_mode;
        uint32_t _PC;
        uint32_t _wait_timeout;
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
        > _wait_data;

}; // threadInfoThread


class threadInfo {
    public:
        threadInfo(void* buf, uint32_t size);
        std::vector<threadInfoThread>& threads();

    private:
        std::vector<threadInfoThread> _threads;
}; // threadInfo

}; // coredump
}; // vita

#endif