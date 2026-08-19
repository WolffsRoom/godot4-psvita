#ifndef VITA_COREDUMP_CALLBACKINFO_H
#define VITA_COREDUMP_CALLBACKINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace vita
{
namespace coredump
{

class callbackInfo;

class callbackInfoCallback {
    friend callbackInfo;
    public:
        uint32_t id();
        uint32_t pid();
        uint32_t threadId();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t func();
        uint32_t argAddress();
        uint32_t notifyCount();
        uint32_t notifyArg();
    private:
        uint32_t _id;
        uint32_t _pid;
        uint32_t _thread_id;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;
        uint32_t _func;
        uint32_t _arg_address;
        uint32_t _notify_count;
        uint32_t _notify_arg;

}; // callbackInfoCallback


class callbackInfo {
    public:
        callbackInfo(void* buf, uint32_t size);
        std::vector<callbackInfoCallback>& callbacks();

    private:
        std::vector<callbackInfoCallback> _callbacks;
}; // callbackInfo

}; // coredump
}; // vita

#endif