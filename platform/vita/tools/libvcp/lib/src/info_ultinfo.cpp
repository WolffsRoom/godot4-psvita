#include "info_ultinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct ultInfo_thread_info {
    uint32_t unk00; // zeroes. probably size
    uint32_t id; // thread addr
    char name[32];
    uint32_t state; // seen: 01, 04
    uint32_t thread_entry; // address
    uint32_t entry_arg; // address
    uint32_t thread_id; // zeroes
    uint32_t exit_code; // seen: 0, 0xFFFFFFFF
    uint32_t fiber_addr; // fiber info
    uint32_t unk7; // seen: zeroes
};


struct ultInfo_runtime {
    uint32_t unk00; // zeroes, probably size
    uint32_t runtime_id; // addr
    char name[32];
    uint32_t worker_threads;
    uint32_t max_ult_threads;
    uint32_t threads_count;
    // threads_count ultInfo_thread_info follow
};

struct ultInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of ultInfo_runtime follows
};

*/

    ultInfo::ultInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _runtimes.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_runtimes[i]._id);
            read_string(_runtimes[i]._name, 32);
            read_var(_runtimes[i]._worker_threads);
            read_var(_runtimes[i]._max_threads);
            uint32_t thread_count;
            read_var(thread_count);
            _runtimes[i]._threads.resize(thread_count);
            for (uint32_t j = 0; j < thread_count; ++j)
            {
                in.seekg(4, std::ios_base::cur);
                read_var(_runtimes[i]._threads[j]._id);
                read_string(_runtimes[i]._threads[j]._name,32);
                read_var(_runtimes[i]._threads[j]._state);
                read_var(_runtimes[i]._threads[j]._entry);
                read_var(_runtimes[i]._threads[j]._arg);
                read_var(_runtimes[i]._threads[j]._thread_id);
                read_var(_runtimes[i]._threads[j]._exit_code);
                read_var(_runtimes[i]._threads[j]._fiber_address);
                in.seekg(4, std::ios_base::cur);
            }
        }
    }

    std::vector<ultInfoRuntime>& ultInfo::runtimes()
    {
        return _runtimes;
    }

    uint32_t ultInfoRuntime::id()
    {
        return _id;
    }

    std::string& ultInfoRuntime::name()
    {
        return _name;
    }

    uint32_t ultInfoRuntime::workerThreads()
    {
        return _worker_threads;
    }

    uint32_t ultInfoRuntime::maxThreads()
    {
        return _max_threads;
    }

    std::vector<ultInfoThread>& ultInfoRuntime::threads()
    {
        return _threads;
    }

    uint32_t ultInfoThread::id()
    {
        return _id;
    }

    std::string& ultInfoThread::name()
    {
        return _name;
    }

    uint32_t ultInfoThread::state()
    {
        return _state;
    }

    std::string ultInfoThread::readableState()
    {
        std::vector<std::string> state;

        switch ((ulthreadState)_state)
        {
            case ulthreadState::ULTHREAD_STATE_INITIALIZED:
                state.push_back("ULTHREAD_STATE_INITIALIZED");
                break;
            case ulthreadState::ULTHREAD_STATE_EXECUTION:
                state.push_back("ULTHREAD_STATE_EXECUTION");
                break;
            case ulthreadState::ULTHREAD_STATE_EXECUTABLE:
                state.push_back("ULTHREAD_STATE_EXECUTABLE");
                break;
            case ulthreadState::ULTHREAD_STATE_WAIT:
                state.push_back("ULTHREAD_STATE_WAIT");
                break;
            case ulthreadState::ULTHREAD_STATE_TERMINATED:
                state.push_back("ULTHREAD_STATE_TERMINATED");
                break;
            case ulthreadState::ULTHREAD_STATE_DELETED:
                state.push_back("ULTHREAD_STATE_DELETED");
                break;
        }
        return utils::join(state, " | ");

    }

    uint32_t ultInfoThread::entry()
    {
        return _entry;
    }

    uint32_t ultInfoThread::arg()
    {
        return _arg;
    }

    uint32_t ultInfoThread::threadId()
    {
        return _thread_id;
    }

    uint32_t ultInfoThread::exitCode()
    {
        return _exit_code;
    }

    uint32_t ultInfoThread::fiberAddress()
    {
        return _fiber_address;
    }

}; // coredump
}; // vita

