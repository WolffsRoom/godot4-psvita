#include <coredumpparser.hpp>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <filesystem>
#include "json.hpp"
#include "tojson.hpp"
#include "zstr/zstr.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

void cat_stream(std::istream& is, std::ostream& os)
{
    const std::streamsize buff_size = 1 << 16;
    char * buff = new char [buff_size];
    while (true)
    {
        is.read(buff, buff_size);
        std::streamsize cnt = is.gcount();
        if (cnt == 0) break;
        os.write(buff, cnt);
    }
    delete [] buff;
}

void decompress_file(const std::string& in, const std::string& out)
{
    std::unique_ptr< std::ofstream > ofs_p;
    if (out.empty())
    {
        std::cout << "WTF" << std::endl;
        return;
    }
    ofs_p = std::unique_ptr< std::ofstream >(new strict_fstream::ofstream(out));
    std::ostream * os_p = ofs_p.get();

    std::unique_ptr< std::istream > is_p = std::unique_ptr< std::istream >(new zstr::ifstream(in));
    cat_stream(*is_p, *os_p);
}

int main( int argc, char** argv )
{
    if ( argc != 2 ) {
        printf( "Usage: dump <file_name>\n" );
        return 1;
    }

    std::string outname = fs::temp_directory_path() / "coredump.tmp";

    decompress_file(argv[1], outname);

    vita::coredump::parser parser(outname);

    if ( !parser.parse() ) {
        printf( "File %s is not found or it is not an PSP2 coredump file\n", argv[1] );
        return 1;
    }

    nlohmann::json j;

    auto cfinfo = parser.getCorefileInfo();

    if (cfinfo)
    {
        j["coredump_info"] = cfinfo.get();

    }
    auto hwinfo = parser.getHwInfo();
    if (hwinfo)
    {
        j["hardware_info"] = hwinfo.get();
    }


    auto bvinfo = parser.getBuildVerInfo();
    if (bvinfo)
    {
        j["build_info"] = bvinfo.get();
    }

    auto sysinfo = parser.getSystemInfo();
    if (sysinfo)
    {
        j["system_info"] = sysinfo.get();
    }

    auto ttyinfo = parser.getTTYInfo();
    if(ttyinfo)
    {
      j["tty"] = ttyinfo->data();
    }

    auto ttyinfo2 = parser.getTTYInfo2();
    if (ttyinfo2)
    {
      j["tty2"] = ttyinfo2->data();
    }

    auto screenshotinfo = parser.getScreenshotInfo();
    if (screenshotinfo)
    {
        if (screenshotinfo->data().size() > 0)
        {
            std::ofstream ofs;
            ofs.open("screen.raw", std::ofstream::out | std::ofstream::binary);
            ofs.write(reinterpret_cast<const char*>(screenshotinfo->data().data()), screenshotinfo->data().size());
            ofs.flush();
            ofs.close();
        }

        j["screenshot"] = screenshotinfo.get();
    }

    auto sysdevinfo = parser.getSysDeviceInfo();
    if (sysdevinfo)
    {
        j["system_devices"] = sysdevinfo.get();
    }


    auto appinfo = parser.getAppInfo();
    if (appinfo)
    {
      j["app_info"] = appinfo.get();
    }

    auto processinfo = parser.getProcessInfo();
    if (processinfo)
    {
      j["process_info"] = processinfo.get();
    }


    auto deviceinfo = parser.getDeviceInfo();
    if (processinfo)
    {
      j["device_info"] = deviceinfo.get();
    }


    auto extnlprocessinfo = parser.getExtnlProcessInfo();
    if (extnlprocessinfo)
    {

      j["external_processes"] = extnlprocessinfo->procs();
    }


    auto fileinfo = parser.getFileInfo();
    if (fileinfo)
    {
      j["files"] = fileinfo->files();
    }

    auto applistinfo = parser.getAppListInfo();
    if (applistinfo)
    {
      j["applications"] = applistinfo->apps();

    }

    auto stackinfo = parser.getStackInfo();
    if (stackinfo)
    {
      j["stack_use"] = stackinfo->stacks();
    }

    auto moduleinfo = parser.getModuleInfo();
    if (moduleinfo)
    {
      j["modules"] = moduleinfo->modules();
    }

    auto memblockinfo = parser.getMemblockInfo();
    if (memblockinfo)
    {
      j["memblocks"] = memblockinfo->memblocks();

    }

    auto budgetinfo = parser.getBudgetInfo();
    if (budgetinfo)
    {
      j["budget"] = budgetinfo->budgets();
    }

    auto libraryinfo = parser.getLibraryInfo();
    if (libraryinfo)
    {
      j["libraries"] = libraryinfo->libraries();
    }



    auto eventinfo = parser.getEventLogInfo();
    if (eventinfo)
    {
      j["eventlog"] = eventinfo->events();
    }
    
    auto semainfo = parser.getSemaphoreInfo();
    if (semainfo)
    {
      j["semaphores"] = semainfo->semaphores();
    }
    
    auto eventflaginfo = parser.getEventflagInfo();
    if (semainfo)
    {
      j["event_flags"] = eventflaginfo->eventflags();
    }

    auto mutexinfo = parser.getMutexInfo();
    if (mutexinfo)
    {
      j["mutexes"] = mutexinfo->mutexes();
    }
    
    auto lwmutexinfo = parser.getLwMutexInfo();
    if (lwmutexinfo)
    {
      j["lightweight_mutexes"] = lwmutexinfo->mutexes();
    }
    
    auto msgpipeinfo = parser.getMsgpipeInfo();
    if (msgpipeinfo)
    {
      j["msgpipes"] = msgpipeinfo->msgpipes();
    }
    
    auto callbackinfo = parser.getCallbackInfo();
    if (callbackinfo)
    {
      j["callbacks"] = callbackinfo->callbacks();
    }
    
    auto timerinfo = parser.getTimerInfo();
    if (timerinfo)
    {
      j["timers"] = timerinfo->timers();
    }
    
    auto rwlockinfo = parser.getRwlockInfo();
    if (rwlockinfo)
    {
      j["rwlocks"] = rwlockinfo->rwlocks();
    }
    
    auto condvarinfo = parser.getCondvarInfo();
    if (condvarinfo)
    {
      j["condvars"] = condvarinfo->condvars();
    }
    
    auto lwcondvarinfo = parser.getLwCondvarInfo();
    if (lwcondvarinfo)
    {
      j["lwcondvars"] = lwcondvarinfo->condvars();
    }
    
    auto simpleeventinfo = parser.getSimpleEventInfo();
    if (simpleeventinfo)
    {
      j["simple_events"] = simpleeventinfo->events();
    }
    
    auto ultinfo = parser.getUltInfo();
    if (ultinfo)
    {
      j["ult_runtimes"] = ultinfo->runtimes();
    }
    
    auto ultsemainfo = parser.getUltSemaphoreInfo();
    if (ultsemainfo)
    {
      j["ult_semaphores"] = ultsemainfo->semaphores();
    }
    
    auto ultcondvarinfo = parser.getUltCondvarInfo();
    if (ultsemainfo)
    {
      j["ult_condvars"] = ultcondvarinfo->condvars();
    }
    
    auto ultmutexinfo = parser.getUltMutexInfo();
    if (ultmutexinfo)
    {
      j["ult_mutexes"] = ultmutexinfo->mutexes();
    }
    
    auto ultrwlockinfo = parser.getUltRwlockInfo();
    if (ultrwlockinfo)
    {
      j["ult_rwlocks"] = ultrwlockinfo->rwlocks();
    }
    
    auto ultwqpoolinfo = parser.getUltWqpoolInfo();
    if (ultwqpoolinfo)
    {
      j["ult_write_queue_pools"] = ultwqpoolinfo->pools();
    }
    
    auto ultqpoolinfo = parser.getUltQpoolInfo();
    if (ultqpoolinfo)
    {
      j["ult_queue_pools"] = ultqpoolinfo->pools();
    }
    
    auto ultqueueinfo = parser.getUltQueueInfo();
    if (ultqueueinfo)
    {
      j["ult_queues"] = ultqueueinfo->queues();
    }
    
    auto fiberinfo = parser.getFiberInfo();
    if (fiberinfo)
    {
      j["fibers"] = fiberinfo->fibers();
    }
    
    auto threadinfo = parser.getThreadInfo();
    if (threadinfo)
    {
      j["threads"] = threadinfo->threads();
    }
    
    auto threadreginfo = parser.getThreadRegInfo();
    if (threadinfo)
    {
      j["thread_registers"] = threadreginfo->threadRegs();
    }
    
    auto gpuinfo = parser.getGpuInfo();
    if (gpuinfo)
    {
      j["gpuinfo"] = gpuinfo.get();
    }
    
    std::cout << j.dump(2, ' ', true, json::error_handler_t::replace) << std::endl;

    fs::remove(outname);

    return 0;
}
