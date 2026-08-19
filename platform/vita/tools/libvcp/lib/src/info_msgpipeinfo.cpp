#include "info_msgpipeinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct syncObjects_thread {
    uint32_t process_id;
    uint32_t thread_id;
};

struct msgpipeInfo_pipe {
    uint32_t unk00; // ?? 0x05
    uint32_t id; // 04
    uint32_t process_id; // 08
    char name[32]; // 0C
    uint32_t attributes; // 2C

    uint32_t ref_count; // 30
    uint32_t buffer_byte_size; // 34
    uint32_t free_buffer_byte_size; // 38
    uint32_t send_thread_count; // 3C
    uint32_t recv_thread_count; // 40
    // send_thread_count syncObjects_thread follow
    // recv_thread_count syncObjects_thread follow
//    uint32_t event_pattern;
//    uint64_t userdata;
};


struct msgpipeInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of msgpipeInfo_pipe follows
};

*/

    msgpipeInfo::msgpipeInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _msgpipes.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_msgpipes[i]._id);
            read_var(_msgpipes[i]._pid);
            read_string(_msgpipes[i]._name, 32);
            read_var(_msgpipes[i]._attributes);
            read_var(_msgpipes[i]._ref_count);
            read_var(_msgpipes[i]._buffer_size);
            read_var(_msgpipes[i]._free_buffer_size);
            uint32_t send_thread_count;
            uint32_t recv_thread_count;
            read_var(send_thread_count);
            read_var(recv_thread_count);

            _msgpipes[i]._send_threads.resize(send_thread_count);
            for (uint32_t j = 0; j < send_thread_count; ++j)
            {
                read_var(_msgpipes[i]._send_threads[j].pid);
                read_var(_msgpipes[i]._send_threads[j].thread_id);
            }

            _msgpipes[i]._recv_threads.resize(recv_thread_count);
            for (uint32_t j = 0; j < recv_thread_count; ++j)
            {
                read_var(_msgpipes[i]._recv_threads[j].pid);
                read_var(_msgpipes[i]._recv_threads[j].thread_id);
            }
            read_var(_msgpipes[i]._event_pattern);
            read_var(_msgpipes[i]._userdata);
        }
    }

    std::vector<msgpipeInfoMsgpipe>& msgpipeInfo::msgpipes()
    {
        return _msgpipes;
    }

    uint32_t msgpipeInfoMsgpipe::id()
    {
        return _id;
    }

    uint32_t msgpipeInfoMsgpipe::pid()
    {
        return _pid;
    }

    std::string& msgpipeInfoMsgpipe::name()
    {
        return _name;
    }

    uint32_t msgpipeInfoMsgpipe::attributes()
    {
        return _attributes;
    }

    std::string msgpipeInfoMsgpipe::readableAttributes()
    {
        std::vector<std::string> attr;

        if (_attributes & 0x00080000)
            attr.push_back("SCE_UID_ATTR_OPEN_FLAG");
        if (_attributes & 0x00000080)
            attr.push_back("SCE_KERNEL_ATTR_OPENABLE");
        if (_attributes & 0x00000100)
            attr.push_back("SCE_KERNEL_EVENT_ATTR_AUTO_RESET");
        else
            attr.push_back("SCE_KERNEL_EVENT_ATTR_MANUAL_RESET");

        if (_attributes & 0x00000003)
            attr.push_back("SCE_KERNEL_MSG_PIPE_ATTR_TH_PRIO");
        if (_attributes & 0x0000000C)
            attr.push_back("SCE_KERNEL_MSG_PIPE_ATTR_TH_FIFO");

        if (_attributes & 0x00000800)
            attr.push_back("SCE_KERNEL_ATTR_NOTIFY_CB_WAKEUP_ONLY");
        else
            attr.push_back("SCE_KERNEL_ATTR_NOTIFY_CB_ALL");

        return utils::join(attr, " | ");
    }

    uint32_t msgpipeInfoMsgpipe::refCount()
    {
        return _ref_count;
    }

    uint32_t msgpipeInfoMsgpipe::bufferSize()
    {
        return _buffer_size;
    }

    uint32_t msgpipeInfoMsgpipe::freeBufferSize()
    {
        return _free_buffer_size;
    }

    std::vector<syncObjectThread>& msgpipeInfoMsgpipe::sendThreads()
    {
        return _send_threads;
    }

    std::vector<syncObjectThread>& msgpipeInfoMsgpipe::recvThreads()
    {
        return _recv_threads;
    }

    uint32_t msgpipeInfoMsgpipe::eventPattern()
    {
        return _event_pattern;
    }

    uint64_t msgpipeInfoMsgpipe::userdata()
    {
        return _userdata;
    }

}; // coredump
}; // vita

