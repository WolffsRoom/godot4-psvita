#include "info_budgetinfo.hpp"

#include <sstream>
#include <iomanip>
#include "memstream.hpp"
#include "utils.h"

namespace vita {
namespace coredump {
/*
struct budgetInfo_budget_partition_segment {
    uint32_t unk00; // zeroes
    uint32_t base; // 04
    uint32_t size; // 08
    uint32_t free_size;  // 0C
    uint32_t min_free_size;  // 10
    uint32_t units; // 14
    uint32_t free_count[9]; // 18
};

struct budgetInfo_budget_partition {
    uint32_t unk00; // zeroes
    uint32_t budget_id; // 04
    char name[32]; // 08
    uint32_t type; // 28
    uint32_t segments_count; // 2C
    // count of budgetInfo_budget_partiton_segment follows
    // partition data is aligned to 0x1E0 (480)
};

struct budgetInfo_budget {
    uint32_t unk00; // zeroes
    uint32_t budget_id;
    char name[32];
    uint32_t partition_count;
    // count of budgetInfo_budget_partiton follows
};

struct budgetInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of budgetInfo_budget follows
};

 */


    budgetInfo::budgetInfo(void *buf, uint32_t size) {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);

        uint32_t count;
        read_var(count);
        _budgets.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_budgets[i]._id);
            read_string(_budgets[i]._name, 32);

            uint32_t p_count;
            read_var(p_count);
            _budgets[i]._partitions.resize(p_count);

            for (uint32_t j = 0; j < p_count; ++j)
            {
                in.seekg(4, std::ios_base::cur);
                read_var(_budgets[i]._partitions[j]._budget_id);
                read_string(_budgets[i]._partitions[j]._name,32);
                read_var(_budgets[i]._partitions[j]._type);

                uint32_t s_count;
                read_var(s_count);
                _budgets[i]._partitions[j]._segments.resize(s_count);
                for(uint32_t k = 0; k < s_count; ++k)
                {
                    in.seekg(4, std::ios_base::cur);
                    read_var(_budgets[i]._partitions[j]._segments[k]._base);
                    read_var(_budgets[i]._partitions[j]._segments[k]._size);
                    read_var(_budgets[i]._partitions[j]._segments[k]._free_size);
                    read_var(_budgets[i]._partitions[j]._segments[k]._min_free_size);
                    read_var(_budgets[i]._partitions[j]._segments[k]._units);
                    in.seekg(4*9, std::ios_base::cur);

                }
                in.seekg(480-(s_count*60), std::ios_base::cur);


            }

        }
    }

    std::vector<budgetInfoBudget> &budgetInfo::budgets() {
        return _budgets;
    }

    uint32_t budgetInfoBudget::id() {
        return _id;
    }

    std::string &budgetInfoBudget::name() {
        return _name;
    }

    std::vector<budgetInfoPartition> &budgetInfoBudget::partitions() {
        return _partitions;
    }

    uint32_t budgetInfoPartition::budgetId() {
        return _budget_id;
    }

    std::string &budgetInfoPartition::name() {
        return _name;
    }

    uint32_t budgetInfoPartition::type() {
        return _type;
    }

    std::string budgetInfoPartition::readableType() {
        std::vector<std::string> type;

        if (_type & 1)
            type.push_back("PHYCONT");
        if ((_type & 0xf0000) == 0x10000)
            type.push_back("LPDDR");
        if ((_type & 0xf0000) == 0x30000)
            type.push_back("CDRAM");

        if ((_type & 0x2) == 0x2)
            type.push_back("SYSTEM");

        if ((_type & 0x4) == 0x4)
            type.push_back("GAME");

        return utils::join(type, " | ");
    }

    std::vector<budgetInfoSegment> &budgetInfoPartition::segments() {
        return _segments;
    }


    uint32_t budgetInfoSegment::base() {
        return _base;
    }

    uint32_t budgetInfoSegment::size() {
        return _size;
    }

    uint32_t budgetInfoSegment::freeSize() {
        return _free_size;
    }

    uint32_t budgetInfoSegment::minFreeSize() {
        return _min_free_size;
    }

    uint32_t budgetInfoSegment::units() {
        return _units;
    }
}
}
