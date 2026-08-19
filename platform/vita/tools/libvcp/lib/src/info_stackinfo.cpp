#include "info_stackinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct stackInfo_info {
    uint32_t unk00; // always zeroes?
    uint32_t thread_id;
    uint32_t peak_stack_use;
    uint32_t current_stack_use;
};

struct stackInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of stackInfo_info follows
};
*/

    stackInfo::stackInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _stacks.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_stacks[i]._thread_id);
            read_var(_stacks[i]._peak_stack_use);
            read_var(_stacks[i]._current_stack_use);
        }
    }

    std::vector<stackInfoStack>& stackInfo::stacks()
    {
        return _stacks;
    }

    uint32_t stackInfoStack::threadId()
    {
        return _thread_id;
    }

    uint32_t stackInfoStack::peakUse()
    {
        return _peak_stack_use;
    }

    uint32_t stackInfoStack::currentUse()
    {
        return _current_stack_use;
    }

}; // coredump
}; // vita

