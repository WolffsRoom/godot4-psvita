#ifndef VITA_COREDUMP_SMPLEVENTINFO_H
#define VITA_COREDUMP_SMPLEVENTINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class simpleeventInfo;

class simpleeventInfoEvent {
    friend simpleeventInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t pattern();
        uint64_t userdata();
        std::vector<syncObjectThread>& threads();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _pattern;
        uint64_t _userdata;
        std::vector<syncObjectThread> _threads;

}; // simpleeventInfoSimpleevent


class simpleeventInfo {
    public:
        simpleeventInfo(void* buf, uint32_t size);
        std::vector<simpleeventInfoEvent>& events();

    private:
        std::vector<simpleeventInfoEvent> _events;
}; // simpleeventInfo

}; // coredump
}; // vita

#endif