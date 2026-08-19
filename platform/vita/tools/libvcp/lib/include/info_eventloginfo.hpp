#ifndef VITA_COREDUMP_EVENTLOGINFO_H
#define VITA_COREDUMP_EVENTLOGINFO_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <variant>

namespace vita
{
namespace coredump
{

class eventLogInfo;

class eventLogInfoEventType1 {
  friend eventLogInfo;
    public:
    uint32_t errorCode();
    uint32_t pid();
    uint32_t budgetType();
    std::string& titleId();
  private:
    uint32_t _error_code;
    uint32_t _pid;
    uint32_t _budget_type;
    std::string _title_id;
};

class eventLogInfoEventType2 {
  friend eventLogInfo;
    public:
    uint32_t errorCode();
  private:
    uint32_t _error_code;
};

class eventLogInfoEventType3 {
  friend eventLogInfo;
    public:
    uint32_t errorCode();
    std::array<std::string, 5>& ips();
  private:
    uint32_t _error_code;
    std::array<std::string, 5> _ips;
};

class eventLogInfoEvent {
    friend eventLogInfo;
    public:
        std::string& appName();
        uint16_t facilityId();
        std::string facility();
        uint16_t type();
        std::string readableType();
        uint32_t pid();
        uint32_t threadGUID();
        uint64_t eventTime();
        std::variant<eventLogInfoEventType1,eventLogInfoEventType2,eventLogInfoEventType3>& event();

    private:
        std::string _app_name;
        uint16_t _facility_id; // event facility id/type. 10001 - Processmgr, 20000 - shell, 20001 - WlanBt
        uint16_t _type; // event index
        uint32_t _pid;
        uint32_t _thread_guid; // thread_id? return of SceSysrootForKernel_D441DC34
        uint64_t _event_time;
        uint32_t _data_size; // 0x1C - processmgr, 0x54 and 0x4 - shell, 0x4 - WlanBt
        std::variant<eventLogInfoEventType1,eventLogInfoEventType2,eventLogInfoEventType3> _event_data;
}; // eventLogInfoEvent


class eventLogInfo {
    public:
        eventLogInfo(void* buf, uint32_t size);
        std::vector<eventLogInfoEvent>& events();

    private:
        std::vector<eventLogInfoEvent> _events;
}; // eventLogInfo

}; // coredump
}; // vita

#endif