#ifndef VITA_COREDUMP_APPINFO_H
#define VITA_COREDUMP_APPINFO_H

#include <cstdint>
#include <string>

namespace vita
{
namespace coredump
{

class appInfo {
    public:
        appInfo(void* buf, uint32_t size);

        std::string& titleId();
        std::string& title();

        std::string& version();
        std::string& sdkVersion();

    private:
        std::string _titleId;
        std::string _title;

        std::string _version;
        std::string _sdkVersion;
}; // appInfo

}; // coredump
}; // vita

#endif