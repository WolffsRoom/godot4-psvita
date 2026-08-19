#include "info_applistinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
struct appListInfo_app {
    uint32_t unk00;
    uint32_t app_id; // 04
    uint32_t parent_app_id; // 08
    uint32_t pid; // 0C
    uint32_t ppid; // 10
    char appid[32]; // 14
    uint32_t budget_id; // 34
    uint32_t launch_mode; // 38
    uint32_t process_attribute; // 3C
    uint32_t mzx_opened_files; // 40
    uint32_t max_directory_level; // 44
    uint32_t path_length; // 48
    char path[align(path_length, 4)];
 
};

struct appListInfo {
    uint32_t unk0; // min. coredump version
    uint32_t count; // max 16
    // count appListInfo_app structs follow
};
*/

    appListInfo::appListInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _apps.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_apps[i]._app_id);
            read_var(_apps[i]._parent_app_id);
            read_var(_apps[i]._pid);
            read_var(_apps[i]._parent_pid);
            read_string(_apps[i]._title_id, 32);
            read_var(_apps[i]._budget_id);
            read_var(_apps[i]._launch_mode);
            read_var(_apps[i]._process_attribute);
            read_var(_apps[i]._max_opened_files);
            read_var(_apps[i]._max_directory_level);

            uint32_t path_length;
            read_var(path_length);
            uint32_t path_size = (path_length + 3) & ~0x03; // align to 4

            read_string(_apps[i]._path, path_size);

        }
    }

    std::vector<appListInfoApp>& appListInfo::apps()
    {
        return _apps;
    }

    uint32_t appListInfoApp::pid()
    {
        return _pid;
    }

    uint32_t appListInfoApp::parentPid()
    {
        return _parent_pid;
    }

    uint32_t appListInfoApp::appId()
    {
        return _app_id;
    }

    uint32_t appListInfoApp::parentAppId()
    {
        return _parent_app_id;
    }

    std::string& appListInfoApp::titleId()
    {
        return _title_id;
    }

    uint32_t appListInfoApp::budgetId()
    {
        return _budget_id;
    }

    uint32_t appListInfoApp::launchMode()
    {
        return _launch_mode;
    }

    uint32_t appListInfoApp::attribute()
    {
        return _process_attribute;
    }

    uint32_t appListInfoApp::maxOpenedFiles()
    {
        return _max_opened_files;
    }

    uint32_t appListInfoApp::maxDirectoryLevel()
    {
        return _max_directory_level;
    }

    std::string& appListInfoApp::path()
    {
        return _path;
    }

}; // coredump
}; // vita

