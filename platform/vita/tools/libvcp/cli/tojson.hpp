#include <coredumpparser.hpp>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "json.hpp"

using json = nlohmann::json;

#define DUMP_HEX0x_FORMAT( width ) \
"0x" << std::setw( width ) << std::setfill( '0' ) << std::hex << std::right
#define DUMP_HEX_FORMAT( width ) \
std::setw( width ) << std::setfill( '0' ) << std::hex << std::right
#define DUMP_DEC_FORMAT( width ) \
std::setw( width ) << std::setfill( '0' ) << std::dec << std::right

template <typename T>
inline std::string hextostring(uint8_t width, T t)
{
  std::ostringstream buf;
  buf << DUMP_HEX_FORMAT(width) << t;
  return buf.str();
}

template <typename T>
inline std::string hextostring0x(uint8_t width, T t)
{
  std::ostringstream buf;
  buf << DUMP_HEX0x_FORMAT(width) << t;
  return buf.str();
}

template <typename T>
inline std::string dectostring(uint8_t width, T t)
{
  std::ostringstream buf;
  buf << DUMP_DEC_FORMAT(width) << t;
  return buf.str();
}

template <typename T>
inline json arraytohex(uint8_t width, T t)
{
  json ret;
  int i = 0;
  for (auto v: t)
  {
    ret[i] = hextostring0x(width, v);
    i++;
  }
  return ret;
}

namespace vita {
namespace coredump {

  void to_json(json& j, vita::coredump::corefileInfo* p) {
        j = json{ {"core_version", p->version()}, {"internal_app", p->isInternal()} };
  }

  void to_json(json& j, vita::coredump::hwInfo* p) {
        j = json{
            {"type", p->productType()},
            {"subtype", p->productSubType()},
            {"qaf_name", p->qafName()},
        };
  }

  void to_json(json& j, vita::coredump::buildVerInfo* p) {
        j = json{
            {"sdk_internal_build", p->sdkInternalBuild()},
            { "system" , json {
                    {"branch", p->systemBranch()},
                    {"revision", p->systemRevision()},
                }
            },
            { "vsh" , json {
                    {"branch", p->vshBranch()},
                    {"revision", p->vshRevision()},
                    {"build", p->vshBuild()},
                }
            },
            { "syscon" , json {
                    {"branch", p->sysconBranch()},
                    {"revision", p->sysconRevision()},
                }
            }
        };
  }
  
  void to_json(json& j, vita::coredump::screenshotInfo* p) {
    j = json{
      {"width", p->width()},
      {"height", p->height()},
    };
    if (p->data().size() > 0)
    {
      j["file"] = "screen.raw";
    }
  }
  
  void to_json(json& j, vita::coredump::systemInfo* p) {
    std::ostringstream psid;
    std::ostringstream hwid;
    for (const auto& b : p->psid())
    {
      psid << DUMP_HEX_FORMAT(2) << +b;
    }
    for (const auto& b : p->hardwareId())
    {
      hwid << DUMP_HEX_FORMAT(2) << +b;
    }
    
    int dtype = p->deviceType();
    int ext_mb = p->extendedMem() / 1024 / 1024;
    
    j = json{
      {"version", json {
        {"string", p->versionString()},
        {"major", p->versionMajor()},
        {"minor", p->versionMinor()},
        {"build", p->versionBuild()},
        
      }
      },
      {"PSID", psid.str()},
      {"CookedPSID", hwid.str()},
      {"ASLR_seed", hextostring0x(8, p->aslrSeed())},
      {"device_type", (dtype == 1 ? "TOOL" : (dtype == 2 ? "DEX" : "CEX"))},
      {"seconds_since_boot", (float)p->timeAwake() / 1000000.f},
      {"extended_memory", (ext_mb > 0 ? "+" : "None") + (ext_mb > 0 ? std::to_string(ext_mb) : "")},
      {"GPI", hextostring0x(8, p->GPI())},
      {"qa_flags", hextostring0x(8, p->qaFlags())},
    };
  }

  void to_json(json& j, vita::coredump::sysDeviceInfo* p) {
    j = json{
      {"game_card", json{
        {"present", p->gc().present()},
        {"title", p->gc().title()},
        {"id", p->gc().id()},
      }},
      {"memory_card", json{
        {"present", p->mc().present()},
        {"title", p->mc().title()},
        {"id", p->mc().id()},
        {"size", p->mc().size()},
        {"manufacturing_date", json{
          {"year", ((p->mc().rawid()[1] << 8) + p->mc().rawid()[2])},
          {"month", (uint32_t)p->mc().rawid()[3]},
          {"day", (uint32_t)p->mc().rawid()[4]},
          {"hour", (uint32_t)p->mc().rawid()[5]},
          {"minute", (uint32_t)p->mc().rawid()[6]},
          {"second", (uint32_t)p->mc().rawid()[7]},
        }}
      }},
      
    };
  }

  void to_json(json& j, vita::coredump::appInfo* p) {
    j = json{ 
      {"title", p->title()},
      {"title_id", p->titleId()},
      {"version", p->version()},
      {"sdk_version", p->sdkVersion()},
      
    };
  }
  
  void to_json(json& j, vita::coredump::processInfo* p) {
    
    j = json{ 
      {"title_id", p->titleId()},
      {"pid", hextostring0x(8, p->pid())},
      {"parent_pid", hextostring0x(8, p->parentPid())},
      {"entry_address", hextostring0x(8, p->entryAddr())},
      {"debug_fingerpint", hextostring0x(8, p->fingerprint())},
      {"path",  p->path()},
      { "attributes" , json {
        {"value",  hextostring0x(8, p->processAttr())},
        {"description", p->readableProcessAttr()},
      }
      },
      { "cpu_affinity" , json {
        {"value",  hextostring0x(8, p->cpuAffinity())},
        {"description", p->readableCpuAffinity()},
      }
      },
      { "stop_reason" , json {
        {"value",  hextostring0x(8, p->stopReason())},
        {"description", p->readableStopReason()},
      }
      },
      {"process_time_seconds",((float)p->time() / 1000000.f)},
      { "arm" , json {
        {"exidx", json {
          {"start",  hextostring0x(8, p->armExidxStart())},
          {"end", hextostring0x(8, p->armExidxEnd())},
        }},
        {"extab", json {
          {"start",  hextostring0x(8, p->armExtabStart())},
          {"end", hextostring0x(8, p->armExtabEnd())},
        }},
        
      }
      },
    };
  }
  
  void to_json(json& j, vita::coredump::deviceInfo* p) {
    
    j = json { 
      {"audio", json {
        {"exlusive_owner_pid", hextostring0x(8, p->audioOwnerPids().exclusive)},
        {"main_out_owner_pid", hextostring0x(8, p->audioOwnerPids().main_out)},
        {"bgm_out_owner_pid", hextostring0x(8, p->audioOwnerPids().bgm_out)},
        {"voice_out_owner_pid", hextostring0x(8, p->audioOwnerPids().voice_out)},
        {"input_owner_pid", hextostring0x(8, p->audioOwnerPids().in)},
      } },
      {"system_chat_owner_app_id", hextostring0x(8, p->systemChatOwnerAppid()) },
      {"display", json {
        {"display_owner_pid1", hextostring0x(8, p->displayOwnerPid()[0])},
        {"display_owner_pid2", hextostring0x(8, p->displayOwnerPid()[1])},
        {"display_backport_owner_pid", hextostring0x(8, p->displayBackPortOwnerPid())},
        {"back_renderer_owner_app_id", hextostring0x(8, p->backRenderPortOwnerAppid())},
        {"render_pids", arraytohex(8, p->renderPids())},
      } },
      {"input", json {
        {"input_owner_pid", hextostring0x(8, p->inputDeviceOwnerPid())},
        {"bluetooth_owner_app_id", hextostring0x(8, p->bluetoothControllerOwnerAppid())},
        {"touch_owner_pids", arraytohex(8, p->touchPids())},
        {"back_renderer_owner_app_id", hextostring0x(8, p->backRenderPortOwnerAppid())},
        {"render_pids", arraytohex(8, p->renderPids())},
      } },
      {"shell_pid", hextostring0x(8, p->shellPid())},
      {"active_app_app_id", hextostring0x(8, p->activeAppAppid())},
    };
  }
  
  void to_json(json& j, vita::coredump::extnlProcessInfoProc p) {
    j = json{ 
      {"name", p.name()},
      {"pid", hextostring0x(8, p.pid())},
      {"parent_pid", hextostring0x(8, p.parentPid())},
      {"entry_address", hextostring0x(8, p.entryAddr())},
      {"debug_fingerpint", hextostring0x(8, p.fingerprint())},
      {"path",  p.path()},
      { "attributes" , json {
        {"value",  hextostring0x(8, p.processAttr())},
        {"description", p.readableProcessAttr()},
      }
      },
      { "cpu_affinity" , json {
        {"value",  hextostring0x(8, p.cpuAffinity())},
        {"description", p.readableCpuAffinity()},
      }
      },
      { "stop_reason" , json {
        {"value",  hextostring0x(8, p.stopReason())},
        {"description", p.readableStopReason()},
      }
      },
      
      { "arm" , json {
        {"exidx", json {
          {"start",  hextostring0x(8, p.armExidxStart())},
          {"end", hextostring0x(8, p.armExidxEnd())},
        }},
        {"extab", json {
          {"start",  hextostring0x(8, p.armExtabStart())},
          {"end", hextostring0x(8, p.armExtabEnd())},
        }},
        
      }
      },
    };
  }
  
  void to_json(json& j, vita::coredump::fileInfoFile p) {
    j = json{ 
      {"fd", hextostring0x(8, p.fd())},
      {"pid", hextostring0x(8, p.pid())},
      {"flags", hextostring0x(8, p.flags())},
      {"attributes", hextostring0x(8, p.attributes())},
      {"mode", hextostring0x(8, p.mode())},
      {"priority", hextostring0x(8, p.priority())},
      {"offset", p.fileOffset()},
      {"size", p.fileSize()},
      {"path", p.path()},
      {"resolved_path", p.resolvedPath()},
      
    };
  }
  
  void to_json(json& j, vita::coredump::appListInfoApp p) {
    j = json{ 
      {"title_id", p.titleId()},
      {"app_id", hextostring0x(8, p.appId())},
      {"parent_app_id", hextostring0x(8, p.parentAppId())},
      {"pid", hextostring0x(8, p.pid())},
      {"parent_pid", hextostring0x(8, p.parentPid())},
      {"budget_pid", hextostring0x(8, p.budgetId())},
      {"launch_mode", hextostring0x(8, p.launchMode())},
      {"attribute", hextostring0x(8, p.attribute())},
      {"max_opened_files", p.maxOpenedFiles()},
      {"max_directory_level", p.maxDirectoryLevel()},
      {"path", p.path()},
      
    };
  }
  
  void to_json(json& j, vita::coredump::stackInfoStack p) {
    j = json{ 
      {"thread_id", hextostring0x(8, p.threadId())},
      {"peak", p.peakUse()},
      {"current", p.currentUse()},
      
    };
  }

  void to_json(json& j, vita::coredump::moduleInfoModule p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"debug_fingerprint", hextostring0x(8, p.fingerprint())},
      {"sdk_version", p.sdkVersion()},
      {"version", p.version()},
      {"type", json {
          {"description", p.readableType()},
          {"value", hextostring0x(2,p.type())},
      }},
      {"flags", json {
        {"description", p.readableFlags()},
        {"value", hextostring0x(8,p.flags())},
      }},
      {"status", json {
        {"description", p.readableStatus()},
        {"value", hextostring0x(8,p.status())},
      }},
      {"module_start", hextostring0x(8,p.moduleStart())},
      {"module_end", hextostring0x(8,p.moduleEnd())},
      {"module_exit", hextostring0x(8,p.moduleExit())},
      { "arm" , json {
        {"exidx", json {
          {"start",  hextostring0x(8, p.armExidxStart())},
          {"end", hextostring0x(8, p.armExidxEnd())},
        }},
        {"extab", json {
          {"start",  hextostring0x(8, p.armExtabStart())},
          {"end", hextostring0x(8, p.armExtabEnd())},
        }},
        
      }
      },
      //{"segments", json{}}
    };
    int i = 0;
    j["segments"] = json::array() ;
    for (auto segment: p.segments())
    {
      j["segments"][i] = json {
        {"attributes", json{
          {"description",  segment.readableAttributes()},
          {"value",  hextostring0x(2,segment.attributes())},
        }},
        {"base_address", hextostring0x(8, segment.baseAddr())},
        {"memory_size", hextostring0x(8, segment.memorySize())},
        {"alignment", hextostring0x(8, segment.alignment())},
      };
      i++;
    }
  }
  
  void to_json(json& j, vita::coredump::memblockInfoBlock p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"type", json {
        {"value", hextostring0x(8, p.type())},
        {"description", p.readableType()},
      }},
      {"header_address", hextostring0x(8, p.headerAddress())},
      {"block_size", hextostring0x(8, p.blockSize())},
      {"allocated_size", hextostring0x(8, p.allocatedSize())},
      {"low_size", hextostring0x(8, p.lowSize())},
      {"high_size", hextostring0x(8, p.highSize())},
      
    };
  }
  
  void to_json(json& j, vita::coredump::budgetInfoBudget p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
    };
    int i = 0;
    j["segments"] = json::array() ;
    for (auto partition: p.partitions())
    {
      j["segments"][i] = json {
        {"name", partition.name()},
        {"budget_id", hextostring0x(8, partition.budgetId())},
        {"type", json{
          {"description",  partition.readableType()},
          {"value",  hextostring0x(8,partition.type())},
        }},
      };
      int k = 0;
      j["segments"][i]["partitions"] = json::array() ;
      for(auto segment: partition.segments())
      {
        j["segments"][i]["partitions"][k] = json {
          {"base", hextostring0x(8, segment.base())},
          {"size", hextostring0x(8, segment.size())},
          {"free", hextostring0x(8, segment.freeSize())},
          {"min_free", hextostring0x(8, segment.minFreeSize())},
          {"units", hextostring0x(8, segment.units())},
        };
        k++;
      }
      i++;
    }
  }
  
  void to_json(json& j, vita::coredump::libraryInfoLibrary p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"module_id", hextostring0x(8, p.moduleId())},
      {"attributes", hextostring0x(8, p.attr())},
    };
    int i = 0;
    j["funcs"] = json::array() ;
    for (auto exportedFunc: p.exportedFuncs())
    {
      j["funcs"][i] = json {
        
        {"nid", hextostring0x(8, exportedFunc.nid)},
        {"address", hextostring0x(8, exportedFunc.address)},
        
      };
      i++;
    }

    i = 0;
    j["vars"] = json::array() ;
    for (auto exportedVar: p.exportedVars())
    {
      j["vars"][i] = json {
        
        {"nid", hextostring0x(8, exportedVar.nid)},
        {"address", hextostring0x(8, exportedVar.address)},
        
      };
      i++;
    }
    i = 0;
    j["client_module_ids"] = json::array() ;
    for (auto id: p.clientModuleIds())
    {
      j["client_module_ids"][i] = hextostring0x(8, id);
      i++;
    }
    i = 0;
    j["tls_offsets"] = json::array() ;
    for (auto tls: p.tlsOffsets())
    {
      j["tls_offsets"][i] = hextostring0x(8, tls);
      i++;
    }
    
    
  }
  
  void to_json(json& j, vita::coredump::eventLogInfoEvent p) {
    j = json{ 
      {"type", json{
        {"description",  p.readableType()},
        {"value",  hextostring0x(8,p.type())},
      }},
      {"facility", json{
        {"description",  p.facility()},
        {"value",  hextostring0x(8,p.facilityId())},
      }},
      {"app_name", p.appName()},
      {"pid",  hextostring0x(8,p.pid())},
      {"thread_guid",  hextostring0x(8,p.threadGUID())},
      {"event_time",  p.eventTime()},
      
    };
    if(auto e = std::get_if<vita::coredump::eventLogInfoEventType1>(&p.event()))
    {
      j["data"] = json {
        {"error_code", hextostring0x(8,e->errorCode())},
        {"pid", hextostring0x(8,e->pid())},
        {"budget_type", hextostring0x(8,e->budgetType())},
        {"title_id",  e->titleId()},
      };
    }
    else if(auto e = std::get_if<vita::coredump::eventLogInfoEventType2>(&p.event()))
    {
      j["data"] = json {
        {"error_code", hextostring0x(8,e->errorCode())},
      };
    }
    else if(auto e = std::get_if<vita::coredump::eventLogInfoEventType3>(&p.event()))
    {
      j["data"] = json {
        {"error_code", hextostring0x(8,e->errorCode())},
        {"ips", e->ips()},
      };
    }
  }
  
  void to_json(json& j, vita::coredump::syncObjectThread thread) {
      j = json {
        {"pid", hextostring0x(8, thread.pid)},
        {"thread_id", hextostring0x(8, thread.thread_id)},
      };
  }
  
  void to_json(json& j, vita::coredump::semaphoreInfoSemaphore p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"pid", hextostring0x(8, p.pid())},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      {"initial_value", p.initialValue()},
      {"value", p.value()},
      {"max_value", p.maxValue()},
      {"threads", p.threads()},
    };

  }
  
  void to_json(json& j, vita::coredump::eventflagInfoEventflag p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"pid", hextostring0x(8, p.pid())},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      {"initial_value", p.initialValue()},
      {"value", p.value()},
      {"threads", p.threads()},
    };
    
  }
  
  void to_json(json& j, vita::coredump::mutexInfoMutex p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"pid", hextostring0x(8, p.pid())},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      {"initial_value", p.initialValue()},
      {"value", p.value()},
      {"thread_owner_id", hextostring0x(8,p.threadOwnerId())},
      {"ceiling_priority", p.ceilingPriority()},
      {"threads", p.threads()},
    };
    
  }

  void to_json(json& j, vita::coredump::lwmutexInfoMutex p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"work_address", hextostring0x(8,p.workAddress())},
      {"initial_value", p.initialValue()},
      {"value", p.value()},
      {"thread_owner_id", hextostring0x(8,p.threadOwnerId())},
      {"threads", p.threads()},
    };
    
  }
  
  void to_json(json& j, vita::coredump::msgpipeInfoMsgpipe p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"pid", hextostring0x(8, p.pid())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      {"buffer_size", p.bufferSize()},
      {"free_buffer_size", p.freeBufferSize()},
      {"event_pattern", hextostring0x(8, p.eventPattern())},
      {"userdata", hextostring0x(16, p.userdata())},
      {"send_threads", p.sendThreads()},
      {"recv_threads", p.recvThreads()},
    };
    
  }
  
  void to_json(json& j, vita::coredump::callbackInfoCallback p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"thread_id", hextostring0x(8, p.threadId())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      {"func", hextostring0x(8,p.func())},
      {"arg_address", hextostring0x(8,p.argAddress())},
      {"notify_count", p.notifyCount()},
      {"notify_arg", hextostring0x(8,p.notifyArg())},
      
    };
  }
  
  
  void to_json(json& j, vita::coredump::timerInfoTimer p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"pid", hextostring0x(8, p.pid())},
      
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      
      {"active_state", hextostring0x(8,p.activeState())},
      {"time_base", p.timeBase()},
      {"time_current", p.timeCurrent()},
      {"time_schedule", p.timeSchedule()},
      {"time_interval", p.timeInterval()},
      {"event_type", hextostring0x(8,p.eventType())},
      {"repeat", p.repeat()},
      {"event_pattern", hextostring0x(8, p.eventPattern())},
      {"userdata", hextostring0x(16, p.userdata())},
      {"threads", p.threads()},
    };
  }
  
  void to_json(json& j, vita::coredump::rwlockInfoRwlock p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"pid", hextostring0x(8, p.pid())},
      {"name", p.name()},
      {"ref_count", p.refCount()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"write_owner_id", hextostring0x(8, p.writeOwnerId())},
      {"lock_count", hextostring0x(8, p.lockCount())},
      {"read_threads", p.readThreads()},
      {"write_threads", p.writeThreads()},
    };
  }
  
  void to_json(json& j, vita::coredump::condvarInfoCondvar p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"pid", hextostring0x(8, p.pid())},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      {"thread_id", hextostring0x(8, p.threadId())},
      {"threads", p.threads()},
    };
  }
  
  void to_json(json& j, vita::coredump::lwcondvarInfoCondvar p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"ref_count", p.refCount()},
      {"mutex_id", hextostring0x(8, p.mutexId())},
      {"work_address", hextostring0x(8, p.workAddress())},
      {"mutex_work_address", hextostring0x(8, p.mutexWorkAddress())},
      {"threads", p.threads()},
    };
  }
  
  void to_json(json& j, vita::coredump::simpleeventInfoEvent p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"pid", hextostring0x(8, p.pid())},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      
      {"pattern", hextostring0x(8, p.pattern())},
      {"userdata", hextostring0x(16, p.userdata())},
      {"threads", p.threads()},
    };
  }
  
  void to_json(json& j, vita::coredump::ultInfoThread p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"state", json {
        {"value", hextostring0x(8, p.state())},
        {"description", p.readableState()},
      }},
      
      {"entry", hextostring0x(8, p.entry())},
      {"arg", hextostring0x(8, p.arg())},
      {"thread_id", hextostring0x(8, p.threadId())},
      {"exit_code", hextostring0x(8, p.exitCode())},
      {"fiber_address", hextostring0x(8, p.fiberAddress())},
    };
  }
  
  void to_json(json& j, vita::coredump::ultInfoRuntime p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"worker_threads", p.workerThreads()},
      {"max_threads", p.maxThreads()},
      {"threads", p.threads()},
    };
  }

  void to_json(json& j, vita::coredump::ultSemaphoreInfoSemaphore p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"current_resources", p.currentResources()},
      {"queue_pool_id", hextostring0x(8, p.queuePoolId())},
    };

    // json can't distinguish between typedef to basic type and basic type
    j["threads"] = json::array();
    int i = 0;
    for (auto t: p.threads())
    {
      j["threads"][i] = hextostring0x(8, t);
    }
  }
  
  void to_json(json& j, vita::coredump::ultCondvarInfoCondvar p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"queue_pool_id", hextostring0x(8, p.mutexId())},
    };
    
    // json can't distinguish between typedef to basic type and basic type
    j["threads"] = json::array();
    int i = 0;
    for (auto t: p.threads())
    {
      j["threads"][i] = hextostring0x(8, t);
      i++;
    }
  }
  
  void to_json(json& j, vita::coredump::ultMutexInfoMutex p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"queue_pool_id", hextostring0x(8, p.queuePoolId())},
      {"lock_status", p.lockStatus()},
      {"recursive_lock_count", p.recursiveLockCount()},
      {"thread_owner_id", hextostring0x(8, p.threadOwnerId())},
    };
    
    // json can't distinguish between typedef to basic type and basic type
    j["threads"] = json::array();
    int i = 0;
    for (auto t: p.threads())
    {
      j["threads"][i] = hextostring0x(8, t);
      i++;
    }
  }
  
  void to_json(json& j, vita::coredump::ultRwlockInfoRwlock p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"queue_pool_id", hextostring0x(8, p.queuePoolId())},
      {"lock_status", p.lockStatus()},
      {"locking_readers", p.lockingReaders()},
      {"thread_owner_id", hextostring0x(8, p.threadOwnerId())},
    };
    
    // json can't distinguish between typedef to basic type and basic type
    j["read_threads"] = json::array();
    int i = 0;
    for (auto t: p.readThreads())
    {
      j["read_threads"][i] = hextostring0x(8, t);
      i++;
    }
    
    j["write_threads"] = json::array();
    i = 0;
    for (auto t: p.writeThreads())
    {
      j["write_threads"][i] = hextostring0x(8, t);
      i++;
    }
  }
  
  void to_json(json& j, vita::coredump::ultWqpoolInfoPool p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"current_waiters", p.currentWaiters()},
      {"max_waiters", p.maxWaiters()},
      {"current_sync_objs", p.currentSyncObjs()},
      {"max_sync_objs", p.maxSyncObjs()},
    };
  }
  
  void to_json(json& j, vita::coredump::ultQpoolInfoPool p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"current_data_resources", p.currentDataResources()},
      {"max_data_resources", p.maxDataResources()},
      {"current_queue_objs", p.currentQueueObjs()},
      {"max_queue_objs", p.maxQueueObjs()},
      {"data_size", hextostring0x(8, p.dataSize())},
    };
  }
  
  void to_json(json& j, vita::coredump::ultQueueInfoQueue p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"queue_pool_id", hextostring0x(8, p.queuePoolId())},
      {"write_queue_pool_id", hextostring0x(8, p.writeQueuePoolId())},
      {"data_size", hextostring0x(8, p.dataSize())},
    };
    
    // json can't distinguish between typedef to basic type and basic type
    j["push_threads"] = json::array();
    int i = 0;
    for (auto t: p.pushThreads())
    {
      j["push_threads"][i] = hextostring0x(8, t);
      i++;
    }
    
    j["pop_threads"] = json::array();
    i = 0;
    for (auto t: p.popThreads())
    {
      j["pop_threads"][i] = hextostring0x(8, t);
      i++;
    }
    j["data"] = json::array();
    i = 0;
    for (auto t: p.data())
    {
      j["data"][i] = hextostring0x(2, (uint32_t)t);
      i++;
    }
  }
  
  void to_json(json& j, vita::coredump::fiberInfoFiber p) {
    j = json{ 
      {"address", hextostring0x(8, p.address())},
      {"name", p.name()},
      {"state", json {
        {"value", hextostring0x(8, p.state())},
        {"description", p.readableState()},
      }},
      {"context_address", hextostring0x(8, p.contextAddress())},
      {"context_size", hextostring0x(8, p.contextSize())},
      {"entry", hextostring0x(8, p.entry())},
      {"thread_id", hextostring0x(8, p.threadId())},
      {"registers", json {
        {"FPSCR", hextostring0x(8, p.FPSCR())},  
      }},
      
    };
    int i = 0;
    j["registers"]["gpr"] = json::array();
    for(auto r: p.GPRs())
    {
      j["registers"]["gpr"][i] = hextostring0x(8, r);
      i++;
    }
    i = 0;
    j["registers"]["neon"] = json::array();
    for(auto r: p.neon())
    {
      j["registers"]["neon"][i] = hextostring0x(16, r);
      i++;
    }
  }
  
  void to_json(json& j, vita::coredump::threadInfoThread p) {
    j = json{ 
      {"id", hextostring0x(8, p.id())},
      {"name", p.name()},
      {"initial_attributes", json {
        {"value", hextostring0x(8, p.initialAttributes())},
        {"description", p.readableInitialAttributes()},
      }},
      {"attributes", json {
        {"value", hextostring0x(8, p.attributes())},
        {"description", p.readableAttributes()},
      }},
      {"status", json {
        {"value", hextostring0x(8, p.status())},
        {"description", p.readableStatus()},
      }},
      {"entry", hextostring0x(8, p.entry())},
      {"stack_top_address", hextostring0x(8, p.stackTopAddress())},
      {"stack_size", hextostring0x(8, p.stackSize())},
      {"stack_used_size", hextostring0x(8, p.stackUsedSize())},
      {"arg_size", hextostring0x(8, p.argSize())},
      {"arg_block_address", hextostring0x(8, p.argBlockAddress())},
      {"initial_priority", hextostring0x(8, p.initialPriority())},
      {"priority", hextostring0x(8, p.priority())},
      {"initial_affinity", json {
        {"value", hextostring0x(8, p.initialAffinity())},
        {"description", p.readableInitialAffinity()},
      }},
      {"affinity", json {
        {"value", hextostring0x(8, p.affinity())},
        {"description", p.readableAffinity()},
      }},
      {"last_cpu_id", hextostring0x(8, p.lastCpuId())},
      {"wait_state_type", json {
        {"value", hextostring0x(8, p.waitStateType())},
        {"description", p.readableWaitStateType()},
      }},
      {"wait_target_id", hextostring0x(8, p.waitTargetId())},
      {"clock_run", p.clockRun()},
      {"stop_reason", json {
        {"value", hextostring0x(8, p.stopReason())},
        {"description", p.readableStopReason()},
      }},
      {"exit_status", hextostring0x(8, p.exitStatus())},
      {"preempted_by_int_count", p.preemptedByIntCount()},
      {"preempted_by_thread_count", p.preemptedByThreadCount()},
      {"voluntarily_release_count", p.voluntarilyReleaseCount()},
      {"change_cpu_count", p.changeCpuCount()},
      {"vfp_mode", hextostring0x(8, p.vfpMode())},
      {"PC", hextostring0x(8, p.PC())},
      {"wait_timeout", p.waitTimeout()},
    };
    if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailCondition>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"mutex_id", hextostring0x(8,e->mutex_id)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailDelay>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"delay", hextostring0x(8,e->delay)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailEvent>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"wait_pattern", hextostring0x(8,e->wait_pattern)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailEventFlag>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"bit_pattern", hextostring0x(8,e->bit_pattern)},
        {"wait_mode", hextostring0x(8,e->wait_mode)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailExceptionForVM>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"type_mask", hextostring0x(8,e->type_mask)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailLWCondition>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"lw_mutex_id", hextostring0x(8,e->lw_mutex_id)},
        {"lw_condition_id", hextostring0x(8,e->lw_condition_id)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailLWMutex>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"lw_condition_id", hextostring0x(8,e->lw_condition_id)},
        {"lock_count", e->lock_count},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailMessagePipe>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"original_size", hextostring0x(8,e->original_size)},
        {"remaining_size", hextostring0x(8,e->remaining_size)},
        {"wait_mode", hextostring0x(8,e->wait_mode)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailMutex>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"condition_id", hextostring0x(8,e->condition_id)},
        {"lock_count", e->lock_count},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailRWLock>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"mode", hextostring0x(8,e->mode)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailSemaphore>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"need_count", hextostring0x(8,e->need_count)},
      };
    }else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailSignal>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"option", hextostring0x(8,e->option)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailMultipleEvents>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"wait_event_list_address", hextostring0x(8,e->wait_event_list_address)},
        {"number_of_events", hextostring0x(8,e->number_of_events)},
        {"number_of_events", hextostring0x(8,e->number_of_events)},
      };
    } else if(auto e = std::get_if<vita::coredump::threadInfoWaitDetailUnknown>(&p.waitData()))
    {
      j["wait_data"] = json {
        {"data1", hextostring0x(8,e->data1)},
        {"data2", hextostring0x(8,e->data2)},
        {"data3", hextostring0x(8,e->data3)},
      };
    }  
  }
  
  void to_json(json& j, vita::coredump::threadRegInfoThreadReg p) {
    j = json{ 
      {"thread_id", hextostring0x(8, p.threadId())},
      {"registers", json {
        {"CPSR", hextostring0x(8, p.CPSR())},  
        {"FPSCR", hextostring0x(8, p.FPSCR())},
        {"TPIDRURO", hextostring0x(8, p.TPIDRURO())},
        {"TPIDRURW", hextostring0x(8, p.TPIDRURW())},  
        {"FPEXC", hextostring0x(8, p.FPEXC())},  
        {"CPACR", hextostring0x(8, p.CPACR())},  
        {"DACR", hextostring0x(8, p.DACR())},  
        {"DBGDSCR", hextostring0x(8, p.DBGDSCR())},  
        {"IFSR", hextostring0x(8, p.IFSR())},  
        {"IFAR", hextostring0x(8, p.IFAR())},  
        {"DFSR", hextostring0x(8, p.DFSR())},  
        {"DFAR", hextostring0x(8, p.DFAR())},  
      }},
      
    };
    int i = 0;
    j["registers"]["gpr"] = json::array();
    for(auto r: p.registers())
    {
      j["registers"]["gpr"][i] = hextostring0x(8, r);
      i++;
    }
    i = 0;
    j["registers"]["neon"] = json::array();
    for(auto r: p.neon())
    {
      j["registers"]["neon"][i] = hextostring0x(16, r);
      i++;
    }
  }

  void to_json(json& j, vita::coredump::gpuInfo* p) {
    j = json{ 
      {"pid", hextostring0x(8, p->pid())},
      {"flags", hextostring0x(8, p->flags())},
      {
        "date",
        dectostring(2,p->day()) + "-" + dectostring(2,p->month()) + "-" + dectostring(4,p->year()) +
        " " + dectostring(2,p->hour()) + ":" + dectostring(2,p->minute()) + ":" + dectostring(2,p->second()) + "." + std::to_string(p->microsecond())
      },
      {"soc_revision", hextostring0x(8, p->socRevision())},
      {"mp_freq", p->mpFreq()},
      {"core_freq", p->coreFreq()},
      {"xbar_freq", p->xbarFreq()},
    };
  }
  
}
}
