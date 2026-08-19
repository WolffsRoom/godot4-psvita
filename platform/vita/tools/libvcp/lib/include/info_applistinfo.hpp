#ifndef VITA_COREDUMP_APPLISTINFO_H
#define VITA_COREDUMP_APPLISTINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace vita
{
namespace coredump
{

class appListInfo;

class appListInfoApp {
    friend appListInfo;
    public:
        uint32_t appId();
        uint32_t parentAppId();

        uint32_t pid();
        uint32_t parentPid();
        std::string& titleId();
        uint32_t budgetId();

        uint32_t launchMode();
        uint32_t attribute();

        uint32_t maxOpenedFiles();
        uint32_t maxDirectoryLevel();

        std::string& path();

    private:

        uint32_t _app_id;
        uint32_t _parent_app_id;
        uint32_t _pid;
        uint32_t _parent_pid;
        std::string _title_id;
        uint32_t _budget_id;
        uint32_t _launch_mode;
        uint32_t _process_attribute;
        uint32_t _max_opened_files;
        uint32_t _max_directory_level;
        std::string _path;
}; // appListInfoApp


class appListInfo {
    public:
        appListInfo(void* buf, uint32_t size);
        std::vector<appListInfoApp>& apps();

    private:
        std::vector<appListInfoApp> _apps;
}; // appListInfo

}; // coredump
}; // vita

#endif