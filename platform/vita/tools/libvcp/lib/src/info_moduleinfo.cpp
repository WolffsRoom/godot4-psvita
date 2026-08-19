#include "info_moduleinfo.hpp"

#include <cstdint>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include "utils.h"
#include "defines.hpp"
#include "memstream.hpp"
#include <array>
#include <iostream>

namespace vita
{
namespace coredump
{
/*
struct moduleInfo_segmentinfo {
  uint32_t size;
  int32_t attributes;
  uint32_t base_addr;
  uint32_t memory_size;
  uint32_t alignment;
};


struct moduleInfo_info {
    uint32_t size; // nulled
    uint32_t module_id; // 04
    uint8_t sdk_version[4]; // 08
    uint8_t version[4]; // 0C
    uint16_t type; // 10
    uint16_t flags; // 12
    uint32_t start; // 14
    uint32_t reference_count; // 18
    uint32_t end; // 1C
    uint32_t exit; // 20
    char name[32];// 24 - coredump seems to copy just 0x1A (0.945 copied 0x1F), but field size is 0x20
    uint32_t status; // 44

    uint32_t fingerprint; // 48

    uint32_t segment_count; // 4C 
    // segment_count of moduleInfo_segmentinfo follow

//    uint32_t arm_exidx_start;
//    uint32_t arm_exidx_end;
//    uint32_t arm_extab_start;
//    uint32_t arm_extab_end;
};

struct moduleInfo {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of moduleInfo_info follows
};

*/

    moduleInfo::moduleInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _modules.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_modules[i]._id);

            uint32_t sdkver;
            read_var(sdkver);

            std::stringstream ss;
            ss << DUMP_HEX_FORMAT(7) << sdkver;

            std::stringstream ss2;
            ss2 << ss.str().substr(0,1) << "." << ss.str().substr(1,3) << "." << ss.str().substr(4,3);

            _modules[i]._sdk_version = ss2.str();

            ss.str("");
            std::array<uint8_t,4> ver;
            read_array(ver, 4);

            ss << std::dec << (uint32_t)ver[1] << "." << (uint32_t)ver[0];
            _modules[i]._version = ss.str();


            read_var(_modules[i]._type);
            read_var(_modules[i]._flags);
            read_var(_modules[i]._module_start);
            read_var(_modules[i]._reference_count);
            read_var(_modules[i]._module_end);
            read_var(_modules[i]._module_exit);

            read_string(_modules[i]._name, 32);

            read_var(_modules[i]._status);
            read_var(_modules[i]._fingerprint);

            uint32_t segment_count;
            read_var(segment_count);

            _modules[i]._segments.resize(segment_count);

            for (uint32_t j = 0; j < segment_count; ++j)
            {
                read_var(_modules[i]._segments[j]._size);
                read_var(_modules[i]._segments[j]._attributes);
                read_var(_modules[i]._segments[j]._base_addr);
                read_var(_modules[i]._segments[j]._memory_size);
                read_var(_modules[i]._segments[j]._alignment);
            }
            read_var(_modules[i]._arm_exidx_start);
            read_var(_modules[i]._arm_exidx_end);
            read_var(_modules[i]._arm_extab_start);
            read_var(_modules[i]._arm_extab_end);
        }
    }

    std::vector<moduleInfoModule>& moduleInfo::modules()
    {
        return _modules;
    }

    std::vector<moduleInfoSegment>& moduleInfoModule::segments()
    {
        return _segments;
    }

    uint32_t moduleInfoModule::id()
    {
        return _id;
    }

    std::string& moduleInfoModule::sdkVersion()
    {
        return _sdk_version;
    }

    std::string& moduleInfoModule::version()
    {
        return _version;
    }

    uint16_t moduleInfoModule::type()
    {
        return _type;
    }

    std::string moduleInfoModule::readableType()
    {
        switch(static_cast<moduleType>(_type))
        {
            case moduleType::FIXED_ELF:
                return std::string("ELF");
            case moduleType::PRX:
                return std::string("PRX");
        }
        return std::string("UNKNOWN");
    }

    uint16_t moduleInfoModule::flags()
    {
        return _flags;
    }

    std::string moduleInfoModule::readableFlags()
    {
        std::vector<std::string> flags;

        PUSH_FLAG(flags, uint16_t, moduleFlags, LOADED_COMMON_DIALOG_BUDGET);
        PUSH_FLAG(flags, uint16_t, moduleFlags, LOADED_DEVTOOL_BUDGET);
        PUSH_FLAG(flags, uint16_t, moduleFlags, SHARED_TEXT_MODULE);
        PUSH_FLAG(flags, uint16_t, moduleFlags, SHAREABLE_TEXT_MODULE);
        PUSH_FLAG(flags, uint16_t, moduleFlags, SYSTEM_MODULE);
        PUSH_FLAG(flags, uint16_t, moduleFlags, PROCESS_MAIN_MODULE);
        PUSH_FLAG(flags, uint16_t, moduleFlags, RELOCATABLE_MODULE);

        return utils::join(flags, " | ");

    }

    uint32_t moduleInfoModule::moduleStart()
    {
        return _module_start;
    }

    uint32_t moduleInfoModule::moduleEnd()
    {
        return _module_end;
    }

    uint32_t moduleInfoModule::moduleExit()
    {
        return _module_exit;
    }

    uint32_t moduleInfoModule::referenceCount()
    {
        return _reference_count;
    }

    std::string& moduleInfoModule::name()
    {
        return _name;
    }

    uint32_t moduleInfoModule::status()
    {
        return _status;
    }

    std::string moduleInfoModule::readableStatus()
    {
        std::vector<std::string> status;

        PUSH_FLAG(status, uint32_t, moduleStatus, LOADED);
        PUSH_FLAG(status, uint32_t, moduleStatus, STARTING);
        PUSH_FLAG(status, uint32_t, moduleStatus, LIVE);
        PUSH_FLAG(status, uint32_t, moduleStatus, STOPPING);
        PUSH_FLAG(status, uint32_t, moduleStatus, STOPPED);

        return utils::join(status, " | ");
    }

    uint32_t moduleInfoModule::fingerprint()
    {
        return _fingerprint;
    }

    uint32_t moduleInfoModule::armExidxStart()
    {
        return _arm_exidx_start;
    }

    uint32_t moduleInfoModule::armExidxEnd()
    {
        return _arm_exidx_end;
    }

    uint32_t moduleInfoModule::armExtabStart()
    {
        return _arm_extab_start;
    }

    uint32_t moduleInfoModule::armExtabEnd()
    {
        return _arm_extab_end;
    }

    uint32_t moduleInfoSegment::size()
    {
        return _size;
    }

    int32_t moduleInfoSegment::attributes()
    {
        return _attributes;
    }

    std::string moduleInfoSegment::readableAttributes()
    {
        std::vector<std::string> attributes;

        PUSH_FLAG(attributes, uint32_t, PRXSegmentAttr, READABLE);
        PUSH_FLAG(attributes, uint32_t, PRXSegmentAttr, WRITABLE);
        PUSH_FLAG(attributes, uint32_t, PRXSegmentAttr, EXECUTABLE);

        return utils::join(attributes, " | ");
    }

    uint32_t moduleInfoSegment::baseAddr()
    {
        return _base_addr;
    }

    uint32_t moduleInfoSegment::memorySize()
    {
        return _memory_size;
    }

    uint32_t moduleInfoSegment::alignment()
    {
        return _alignment;
    }

}; // coredump
}; // vita

