#ifndef VITA_COREDUMP_BUDGETINFO_H
#define VITA_COREDUMP_BUDGETINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace vita
{
namespace coredump
{

class budgetInfo;

class budgetInfoSegment {
    friend budgetInfo;
    public:
        uint32_t base();
        uint32_t size();
        uint32_t freeSize();
        uint32_t minFreeSize();
        uint32_t units();
    private:
        uint32_t _base;
        uint32_t _size;
        uint32_t _free_size;
        uint32_t _min_free_size;
        uint32_t _units;
}; // budgetInfoPartition

class budgetInfoPartition {
    friend budgetInfo;
    public:
        uint32_t budgetId();
        std::string& name();
        uint32_t type();
        std::string readableType();
        std::vector<budgetInfoSegment>& segments();
    private:
        uint32_t _budget_id;
        std::string _name;
        uint32_t _type;
        std::vector<budgetInfoSegment> _segments;
}; // budgetInfoPartition


class budgetInfoBudget {
    friend budgetInfo;
    public:
        uint32_t id();
        std::string& name();
        std::vector<budgetInfoPartition>& partitions();
    private:
        uint32_t _id;
        std::string _name;
        std::vector<budgetInfoPartition> _partitions;
}; // budgetInfoBudget

class budgetInfo {
    public:
        budgetInfo(void* buf, uint32_t size);
        std::vector<budgetInfoBudget>& budgets();

    private:
        std::vector<budgetInfoBudget> _budgets;
}; // budgetInfo

}; // coredump
}; // vita

#endif