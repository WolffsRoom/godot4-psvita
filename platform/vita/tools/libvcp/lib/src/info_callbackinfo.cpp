#include "info_callbackinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct callbackInfo_info {
    uint32_t unk00; // size
    uint32_t id; // 04
    uint32_t process_id; // 08
    uint32_t thread_id; // 0C
    char name[32]; // 10
    uint32_t attributes; // 30
    uint32_t ref_count; // 34
    uint32_t callback_func; // 38
    uint32_t arg_addr; // 3C
    uint32_t notify_count; // 40
    uint32_t notify_arg; // 44
};


struct callbackInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of callbackInfo_info follows
};

*/

    callbackInfo::callbackInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _callbacks.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_callbacks[i]._id);
            read_var(_callbacks[i]._pid);
            read_var(_callbacks[i]._thread_id);
            read_string(_callbacks[i]._name, 32);
            read_var(_callbacks[i]._attributes);
            read_var(_callbacks[i]._ref_count);
            read_var(_callbacks[i]._func);
            read_var(_callbacks[i]._arg_address);
            read_var(_callbacks[i]._notify_count);
            read_var(_callbacks[i]._notify_arg);
        }
    }

    std::vector<callbackInfoCallback>& callbackInfo::callbacks()
    {
        return _callbacks;
    }

    uint32_t callbackInfoCallback::id()
    {
        return _id;
    }

    uint32_t callbackInfoCallback::pid()
    {
        return _pid;
    }

    uint32_t callbackInfoCallback::threadId()
    {
        return _thread_id;
    }

    std::string& callbackInfoCallback::name()
    {
        return _name;
    }

    uint32_t callbackInfoCallback::attributes()
    {
        return _attributes;
    }

    std::string callbackInfoCallback::readableAttributes()
    {
        std::vector<std::string> attr;

        if (_attributes & 0x00080000)
            attr.push_back("SCE_UID_ATTR_OPEN_FLAG");
        if (_attributes & 0x00000080)
            attr.push_back("SCE_KERNEL_ATTR_OPENABLE");

        if (_attributes == 0x00000000)
            attr.push_back("NONE");

        return utils::join(attr, " | ");
    }

    uint32_t callbackInfoCallback::refCount()
    {
        return _ref_count;
    }

    uint32_t callbackInfoCallback::func()
    {
        return _func;
    }

    uint32_t callbackInfoCallback::argAddress()
    {
        return _arg_address;
    }

    uint32_t callbackInfoCallback::notifyCount()
    {
        return _notify_count;
    }

    uint32_t callbackInfoCallback::notifyArg()
    {
        return _notify_arg;
    }


}; // coredump
}; // vita

