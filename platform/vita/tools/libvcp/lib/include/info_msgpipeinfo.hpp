#ifndef VITA_COREDUMP_MSGPIPEINFO_H
#define VITA_COREDUMP_MSGPIPEINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "sync_thread.hpp"

namespace vita
{
namespace coredump
{

class msgpipeInfo;

class msgpipeInfoMsgpipe {
    friend msgpipeInfo;
    public:
        uint32_t id();
        uint32_t pid();
        std::string& name();
        uint32_t attributes();
        std::string readableAttributes();
        uint32_t refCount();
        uint32_t bufferSize();
        uint32_t freeBufferSize();
        std::vector<syncObjectThread>& sendThreads();
        std::vector<syncObjectThread>& recvThreads();
        uint32_t eventPattern();
        uint64_t userdata();
    private:
        uint32_t _id;
        uint32_t _pid;
        std::string _name;
        uint32_t _attributes;
        uint32_t _ref_count;
        uint32_t _buffer_size;
        uint32_t _free_buffer_size;

        std::vector<syncObjectThread> _send_threads;
        std::vector<syncObjectThread> _recv_threads;
        uint32_t _event_pattern;
        uint64_t _userdata;

}; // msgpipeInfoMsgpipe


class msgpipeInfo {
    public:
        msgpipeInfo(void* buf, uint32_t size);
        std::vector<msgpipeInfoMsgpipe>& msgpipes();

    private:
        std::vector<msgpipeInfoMsgpipe> _msgpipes;
}; // msgpipeInfo

}; // coredump
}; // vita

#endif