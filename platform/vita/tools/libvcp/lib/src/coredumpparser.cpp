#include "elfio/elfio.hpp"
#include <coredumpparser.hpp>

#include <sstream>

#include "note_types.h"

#define INFO_GETTER(name, type, note_type) \
    std::unique_ptr<type> parser::name() \
    { \
        std::pair<void*, uint32_t> data = _getNoteData(elf::note::note_type); \
        if ( data.first != nullptr ) \
        { \
            return std::unique_ptr<type>(new type(data.first, data.second)); \
        } \
        return nullptr; \
    }

namespace vita
{
namespace coredump
{
    parser::parser(std::string& filename) : _filename(filename) {
        _reader = new ::ELFIO::elfio();
    }
    parser::parser(const char* filename) {
        _filename = std::string(filename);
        _reader = new ::ELFIO::elfio();
    }

    parser::~parser() {}


    bool parser::parse()
    {
        if (!_reader->load(_filename))
        {
            return false;
        }
        ELFIO::Elf_Half n = _reader->segments.size();

        for ( ELFIO::Elf_Half i = 0; i < n; ++i )
        {
            ELFIO::segment* seg = _reader->segments[i];
            if (seg->get_type() == ELFIO::PT_NOTE)
            {

                ELFIO::note_segment_accessor notes( *_reader, seg );
                ELFIO::Elf_Word  no_notes = notes.get_notes_num();

                if ( no_notes > 0 )
                {
                    for ( ELFIO::Elf_Word j = 0; j < no_notes; ++j ) // For all notes
                    {
                        ELFIO::Elf_Word    type;
                        std::string name;
                        void*       desc;
                        ELFIO::Elf_Word    descsz;

                        if ( notes.get_note( j, type, name, desc, descsz ) )
                        {
                            _notes.emplace(static_cast<elf::note>(type), std::make_pair(j, i));
                        }
                    }
                }
            }
        }
        return true;
    }


    std::pair<void*, uint32_t> parser::_getNoteData(elf::note note_type)
    {
        if (_notes.find(note_type) != _notes.end())
        {
            std::pair<uint32_t, uint32_t> idx = _getNoteIdx(note_type);

            ELFIO::segment* seg = _reader->segments[idx.second];
            ELFIO::note_segment_accessor notes( *_reader, seg );

            ELFIO::Elf_Word    type;
            std::string name;
            void*       desc;
            ELFIO::Elf_Word    descsz;
            if ( notes.get_note( idx.first, type, name, desc, descsz ) )
            {
                return std::make_pair(desc, descsz);
            }
        }
        return std::make_pair(nullptr, 0);;
    }

    std::pair<uint32_t, uint32_t>& parser::_getNoteIdx(elf::note type)
    {
        return _notes.at(type);
    }

    INFO_GETTER(getCorefileInfo, corefileInfo, COREFILE_INFO)
    INFO_GETTER(getHwInfo, hwInfo, HW_INFO)
    INFO_GETTER(getBuildVerInfo, buildVerInfo, BUILD_VER_INFO)
    INFO_GETTER(getSystemInfo, systemInfo, SYSTEM_INFO)
    INFO_GETTER(getTTYInfo, ttyInfo, TTY_INFO)
    INFO_GETTER(getTTYInfo2, ttyInfo, TTY_INFO2)
    INFO_GETTER(getScreenshotInfo, screenshotInfo, SCREENSHOT_INFO)
    INFO_GETTER(getSysDeviceInfo, sysDeviceInfo, SYS_DEVICE_INFO)
    INFO_GETTER(getAppInfo, appInfo, APP_INFO)
    INFO_GETTER(getProcessInfo, processInfo, PROCESS_INFO)
    INFO_GETTER(getDeviceInfo, deviceInfo, DEVICE_INFO)
    INFO_GETTER(getExtnlProcessInfo, extnlProcessInfo, EXTNL_PROC_INFO)
    INFO_GETTER(getFileInfo, fileInfo, FILE_INFO)
    INFO_GETTER(getAppListInfo, appListInfo, APP_LIST_INFO)
    INFO_GETTER(getStackInfo, stackInfo, STACK_INFO)
    INFO_GETTER(getModuleInfo, moduleInfo, MODULE_INFO)
    INFO_GETTER(getMemblockInfo, memblockInfo, MEM_BLK_INFO)
    INFO_GETTER(getBudgetInfo, budgetInfo, BUDGET_INFO)
    INFO_GETTER(getLibraryInfo, libraryInfo, LIBRARY_INFO)
    INFO_GETTER(getEventLogInfo, eventLogInfo, EVENT_LOG_INFO)
    INFO_GETTER(getSemaphoreInfo, semaphoreInfo, SEMAPHORE_INFO)
    INFO_GETTER(getEventflagInfo, eventflagInfo, EVENTFLAG_INFO)
    INFO_GETTER(getMutexInfo, mutexInfo, MUTEX_INFO)
    INFO_GETTER(getLwMutexInfo, lwmutexInfo, LWMUTEX_INFO)
    INFO_GETTER(getMsgpipeInfo, msgpipeInfo, MESG_PIPE_INFO)
    INFO_GETTER(getCallbackInfo, callbackInfo, CALLBACK_INFO)
    INFO_GETTER(getTimerInfo, timerInfo, TIMER_INFO)
    INFO_GETTER(getRwlockInfo, rwlockInfo, RWLOCK_INFO)
    INFO_GETTER(getCondvarInfo, condvarInfo, CONDVAR_INFO)
    INFO_GETTER(getLwCondvarInfo, lwcondvarInfo, LWCONDVAR_INFO)
    INFO_GETTER(getSimpleEventInfo, simpleeventInfo, SMPL_EVENT_INFO)
    INFO_GETTER(getUltInfo, ultInfo, ULT_INFO)
    INFO_GETTER(getUltSemaphoreInfo, ultSemaphoreInfo, ULT_SEMA_INFO)
    INFO_GETTER(getUltCondvarInfo, ultCondvarInfo, ULT_COND_INFO)
    INFO_GETTER(getUltMutexInfo, ultMutexInfo, ULT_MUTEX_INFO)
    INFO_GETTER(getUltRwlockInfo, ultRwlockInfo, ULT_RWLOCK_INFO)
    INFO_GETTER(getUltWqpoolInfo, ultWqpoolInfo, ULT_WQPOOL_INFO)
    INFO_GETTER(getUltQpoolInfo, ultQpoolInfo, ULT_Q_POOL_INFO)
    INFO_GETTER(getUltQueueInfo, ultQueueInfo, ULT_QUEUE_INFO)
    INFO_GETTER(getFiberInfo, fiberInfo, FIBER_INFO)
    INFO_GETTER(getThreadInfo, threadInfo, THREAD_INFO)
    INFO_GETTER(getThreadRegInfo, threadRegInfo, THREAD_REG_INFO)
    INFO_GETTER(getGpuInfo, gpuInfo, GPU_INFO)

}; // coredump
}; // vita

