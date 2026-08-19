#ifndef VITA_COREDUMP_PARSER_H
#define VITA_COREDUMP_PARSER_H

#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <utility>
#include <memory>

//#include "elfio/elfio.hpp"

#include "info_corefileinfo.hpp"
#include "info_hwinfo.hpp"
#include "info_buildverinfo.hpp"
#include "info_systeminfo.hpp"
#include "info_ttyinfo.hpp"
#include "info_screenshotinfo.hpp"
#include "info_sysdeviceinfo.hpp"
#include "info_appinfo.hpp"
#include "info_processinfo.hpp"
#include "info_deviceinfo.hpp"
#include "info_extnlprocessinfo.hpp"
#include "info_fileinfo.hpp"
#include "info_applistinfo.hpp"
#include "info_stackinfo.hpp"
#include "info_moduleinfo.hpp"
#include "info_memblockinfo.hpp"
#include "info_budgetinfo.hpp"
#include "info_libraryinfo.hpp"
#include "info_eventloginfo.hpp"
#include "info_semaphoreinfo.hpp"
#include "info_eventflaginfo.hpp"
#include "info_mutexinfo.hpp"
#include "info_lwmutexinfo.hpp"
#include "info_msgpipeinfo.hpp"
#include "info_callbackinfo.hpp"
#include "info_timerinfo.hpp"
#include "info_rwlockinfo.hpp"
#include "info_condvarinfo.hpp"
#include "info_lwcondvarinfo.hpp"
#include "info_simpleeventinfo.hpp"
#include "info_ultinfo.hpp"
#include "info_ultsemaphoreinfo.hpp"
#include "info_ultcondvarinfo.hpp"
#include "info_ultmutexinfo.hpp"
#include "info_ultrwlockinfo.hpp"
#include "info_ultwqpoolinfo.hpp"
#include "info_ultqpoolinfo.hpp"
#include "info_ultqueueinfo.hpp"
#include "info_fiberinfo.hpp"
#include "info_threadinfo.hpp"
#include "info_threadreginfo.hpp"
#include "info_gpuinfo.hpp"

namespace elf {
    enum class note;
};

namespace ELFIO {
    class elfio;
}

namespace vita
{
namespace coredump
{

class parser
{
    public:
        parser(std::string& filename);
        parser(const char* filename);
        ~parser();

        bool parse();

        std::unique_ptr<corefileInfo> getCorefileInfo();
        std::unique_ptr<hwInfo> getHwInfo();
        std::unique_ptr<buildVerInfo> getBuildVerInfo();
        std::unique_ptr<systemInfo> getSystemInfo();
        std::unique_ptr<ttyInfo> getTTYInfo();
        std::unique_ptr<ttyInfo> getTTYInfo2();
        std::unique_ptr<screenshotInfo> getScreenshotInfo();
        std::unique_ptr<sysDeviceInfo> getSysDeviceInfo();
        std::unique_ptr<appInfo> getAppInfo();
        std::unique_ptr<processInfo> getProcessInfo();
        std::unique_ptr<deviceInfo> getDeviceInfo();
        std::unique_ptr<extnlProcessInfo> getExtnlProcessInfo();
        std::unique_ptr<fileInfo> getFileInfo();
        std::unique_ptr<appListInfo> getAppListInfo();
        std::unique_ptr<stackInfo> getStackInfo();
        std::unique_ptr<moduleInfo> getModuleInfo();
        std::unique_ptr<memblockInfo> getMemblockInfo();
        std::unique_ptr<budgetInfo> getBudgetInfo();
        std::unique_ptr<libraryInfo> getLibraryInfo();
        std::unique_ptr<eventLogInfo> getEventLogInfo();
        std::unique_ptr<semaphoreInfo> getSemaphoreInfo();
        std::unique_ptr<eventflagInfo> getEventflagInfo();
        std::unique_ptr<mutexInfo> getMutexInfo();
        std::unique_ptr<lwmutexInfo> getLwMutexInfo();
        std::unique_ptr<msgpipeInfo> getMsgpipeInfo();
        std::unique_ptr<callbackInfo> getCallbackInfo();
        std::unique_ptr<timerInfo> getTimerInfo();
        std::unique_ptr<rwlockInfo> getRwlockInfo();
        std::unique_ptr<condvarInfo> getCondvarInfo();
        std::unique_ptr<lwcondvarInfo> getLwCondvarInfo();
        std::unique_ptr<simpleeventInfo> getSimpleEventInfo();
        std::unique_ptr<ultInfo> getUltInfo();
        std::unique_ptr<ultSemaphoreInfo> getUltSemaphoreInfo();
        std::unique_ptr<ultCondvarInfo> getUltCondvarInfo();
        std::unique_ptr<ultMutexInfo> getUltMutexInfo();
        std::unique_ptr<ultRwlockInfo> getUltRwlockInfo();
        std::unique_ptr<ultWqpoolInfo> getUltWqpoolInfo();
        std::unique_ptr<ultQpoolInfo> getUltQpoolInfo();
        std::unique_ptr<ultQueueInfo> getUltQueueInfo();
        std::unique_ptr<fiberInfo> getFiberInfo();
        std::unique_ptr<threadInfo> getThreadInfo();
        std::unique_ptr<threadRegInfo> getThreadRegInfo();
        std::unique_ptr<gpuInfo> getGpuInfo();

    private:
        ::ELFIO::elfio *_reader;
        std::string _filename;
        std::unordered_map<elf::note, std::pair<uint32_t,uint32_t>> _notes;
        std::pair<uint32_t,uint32_t>& _getNoteIdx(elf::note type);
        std::pair<void*,uint32_t> _getNoteData(elf::note note_type);
}; // parser

}; // coredump
}; // vita

#endif