#ifndef NOTE_TYPES_H
#define NOTE_TYPES_H

namespace elf
{
    enum class note
    {
        COREFILE_INFO   = 0x00001000,
        SYSTEM_INFO     = 0x00001001,

        PROCESS_INFO    = 0x00001002,
        THREAD_INFO     = 0x00001003,
        THREAD_REG_INFO = 0x00001004,
        MODULE_INFO     = 0x00001005,
        LIBRARY_INFO    = 0x00001006,
        MEM_BLK_INFO    = 0x00001007,

        FILE_INFO       = 0x00001009,
        SEMAPHORE_INFO  = 0x0000100a,
        EVENTFLAG_INFO  = 0x0000100b,
        MUTEX_INFO      = 0x0000100c,
        LWMUTEX_INFO    = 0x0000100d,

        MESG_PIPE_INFO  = 0x00001010,
        CALLBACK_INFO   = 0x00001011,
        TIMER_INFO      = 0x00001013,
        RWLOCK_INFO     = 0x00001014,

        CONDVAR_INFO    = 0x00001015,
        LWCONDVAR_INFO  = 0x00001016,
        FIBER_INFO      = 0x00001017,
        ULT_INFO        = 0x00001018,
        META_DATA_INFO  = 0x00001019,
        HW_INFO         = 0x0000101a,
        STACK_INFO      = 0x0000101b,
        APP_INFO        = 0x0000101c,
        BUILD_VER_INFO  = 0x0000101d,
        EXTNL_PROC_INFO = 0x0000101e,
        BUDGET_INFO     = 0x0000101f,
        APP_LIST_INFO   = 0x00001020,
        DEVICE_INFO     = 0x00001021,

        USER_INFO       = 0x00001022, // raw data from user coredump handler

        ULT_SEMA_INFO   = 0x00001023,
        ULT_MUTEX_INFO  = 0x00001024,
        ULT_Q_POOL_INFO = 0x00001025,
        ULT_WQPOOL_INFO = 0x00001026,
        ULT_QUEUE_INFO  = 0x00001027,
        ULT_RWLOCK_INFO = 0x00001028,
        ULT_COND_INFO   = 0x00001029,

        TTY_INFO        = 0x0000102a,
        SCREENSHOT_INFO = 0x0000102b, // fulldump only

        EVENT_LOG_INFO  = 0x0000102c,
        SYSTEM_INFO2    = 0x0000102d, // was MINI_SS_INFO = on 2.60, minidump only
        SUMMARY_INFO    = 0x0000102e,
        SYS_DEVICE_INFO = 0x0000102f,

        SMPL_EVENT_INFO = 0x00001030,

        TTY_INFO2       = 0x00001031,
        GPU_INFO        = 0x00002000, // gpu crash only
        GPU_ACT_INFO    = 0x00002001, // seems to be present, but filled with 0x810 sized data on devkit in devmode only on specific gpu bug?
        KERNEL_INFO     = 0x00003000, // seems to be written by SceCoredumpForDriver_13EF8516 with NPXS19999 as app
    };
};



#endif
