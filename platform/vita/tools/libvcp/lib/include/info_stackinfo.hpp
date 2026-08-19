#ifndef VITA_COREDUMP_STACKINFO_H
#define VITA_COREDUMP_STACKINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace vita
{
namespace coredump
{

class stackInfo;

class stackInfoStack {
    friend stackInfo;
    public:
        uint32_t threadId();

        uint32_t peakUse();
        uint32_t currentUse();


    private:
        uint32_t _thread_id;
        uint32_t _peak_stack_use;
        uint32_t _current_stack_use;
}; // stackInfoStack


class stackInfo {
    public:
        stackInfo(void* buf, uint32_t size);
        std::vector<stackInfoStack>& stacks();

    private:
        std::vector<stackInfoStack> _stacks;
}; // stackInfo

}; // coredump
}; // vita

#endif