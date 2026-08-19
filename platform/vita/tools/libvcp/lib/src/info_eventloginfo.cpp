#include "info_eventloginfo.hpp"

#include <cstdint>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include "utils.h"
#include "defines.hpp"
#include "memstream.hpp"
#include <iostream>

namespace vita
{
namespace coredump
{
/*
struct eventLogInfo_eventtype1 { // 0x1C
    uint32_t error_code;
    uint32_t pid;
    uint32_t budget_type;
    uint32_t data_0x4C;    // 0xA
    char titleid[0xC];
};

struct eventLogInfo_eventtype2 { // 0x04
    uint32_t error_code;
};

struct eventLogInfo_eventtype3 { // 0x54
    uint32_t error_code;
    char ip1[0x10];
    char ip2[0x10];
    char ip3[0x10];
    char ip4[0x10];
    char ip5[0x10];
};

struct eventLogInfo_event { // variable size
    uint32_t size;
    uint32_t app_name_size;
    char app_name[0xC]; // hardcoded in crashdump
    uint16_t ev_id; // event facility id/type. 10001 - Processmgr, 20000 - shell, 20001 - WlanBt
    uint16_t type; // event type
    uint32_t pid;
    uint32_t thread_guid; // return of SceSysrootForKernel_D441DC34
    uint32_t rsvd[4];
    uint64_t event_time;
    uint32_t unk3; // param5 of 0x912CF2BA (param3 of 0x95B38C6C). 0 from Processmgr, shell and WlanBt
    uint32_t data_size; // 0x1C - processmgr, 0x54 and 0x4 - shell, 0x4 - WlanBt
};

struct eventLogInfo {
    uint32_t unk00; // coredump minver
    uint32_t unk04; // zeroes
    uint32_t event_count;
    // event_count events follow
    uint8_t  events[0x800]; // copy of sceEventLogGetInfoForDriver (0xCC5365D3) 0x800 sized buffer
};

*/

    eventLogInfo::eventLogInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(8, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _events.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(8, std::ios_base::cur);

            read_string(_events[i]._app_name, 12);

            read_var(_events[i]._facility_id);
            read_var(_events[i]._type);
            read_var(_events[i]._pid);
            read_var(_events[i]._thread_guid);
            in.seekg(4*4, std::ios_base::cur);
            read_var(_events[i]._event_time);
            in.seekg(4, std::ios_base::cur);

            read_var(_events[i]._data_size);

            switch(_events[i]._data_size)
            {
                case 0x1c:
                {
                  _events[i]._event_data = eventLogInfoEventType1{};
                  auto d = std::get_if<eventLogInfoEventType1>(&_events[i]._event_data);
                  read_var(d->_error_code);
                  read_var(d->_pid);
                  read_var(d->_budget_type);
                  in.seekg(4, std::ios_base::cur);
                  read_string(d->_title_id, 12);
                  break;
                }
                case 0x04:
                {
                  _events[i]._event_data = eventLogInfoEventType2{};
                  auto d = std::get_if<eventLogInfoEventType2>(&_events[i]._event_data);
                  read_var(d->_error_code);
                  break;
                }
                case 0x54:
                {
                  _events[i]._event_data = eventLogInfoEventType3{};
                  auto d = std::get_if<eventLogInfoEventType3>(&_events[i]._event_data);
                  read_var(d->_error_code);
                  read_string(d->_ips[0],16);
                  read_string(d->_ips[1],16);
                  read_string(d->_ips[2],16);
                  read_string(d->_ips[3],16);
                  read_string(d->_ips[4],16);
                  break;
                }
            }
        }
    }

    std::vector<eventLogInfoEvent>& eventLogInfo::events()
    {
        return _events;
    }

    uint16_t eventLogInfoEvent::facilityId()
    {
        return _facility_id;
    }

    std::string eventLogInfoEvent::facility()
    {
        switch(_facility_id)
        {
            case 10001:
                return std::string("Processmgr");
            case 20000:
                return std::string("Shell");
            case 20001:
                return std::string("WlanBt");
            default:
                return std::string("unknown");
        }
    }

    uint16_t eventLogInfoEvent::type()
    {
        return _type;
    }

    std::string eventLogInfoEvent::readableType()
    {
        switch(_facility_id)
        {
            case 10001:
                switch(_type)
                {
                    case 1:
                        return std::string("process create");
                    case 2:
                        return std::string("process exit");
                    case 3:
                        return std::string("process kill");
                    case 4:
                        return std::string("process coredump");
                    default:
                        return std::string("unknown");
                }
            case 20001:
                switch(_type)
                {
                    case 1:
                        return std::string("suspend");
                    case 2:
                        return std::string("resume");
                    default:
                        return std::string("unknown");
                }
            case 20000:
            default:
                return std::string("unknown");
        }
    }

    std::string& eventLogInfoEvent::appName()
    {
        return _app_name;
    }

    uint32_t eventLogInfoEvent::pid()
    {
        return _pid;
    }

    uint32_t eventLogInfoEvent::threadGUID()
    {
        return _thread_guid;
    }

    uint64_t eventLogInfoEvent::eventTime()
    {
        return _event_time;
    }

    std::variant<eventLogInfoEventType1,eventLogInfoEventType2,eventLogInfoEventType3>& eventLogInfoEvent::event()
    {
        return _event_data;
    }

    uint32_t eventLogInfoEventType1::errorCode()
    {
        return _error_code;
    }

    uint32_t eventLogInfoEventType1::pid()
    {
        return _pid;
    }

    uint32_t eventLogInfoEventType1::budgetType()
    {
        return _budget_type;
    }

    std::string& eventLogInfoEventType1::titleId()
    {
        return _title_id;
    }

    uint32_t eventLogInfoEventType2::errorCode()
    {
        return _error_code;
    }

    uint32_t eventLogInfoEventType3::errorCode()
    {
        return _error_code;
    }

    std::array<std::string, 5>& eventLogInfoEventType3::ips()
    {
        return _ips;
    }


}; // coredump
}; // vita

