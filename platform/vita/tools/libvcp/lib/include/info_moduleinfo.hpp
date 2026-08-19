#ifndef VITA_COREDUMP_MODULEINFO_H
#define VITA_COREDUMP_MODULEINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace vita
{
namespace coredump
{

class moduleInfo;

class moduleInfoSegment {
    friend moduleInfo;
    public:
      uint32_t size();
      int32_t attributes();
      std::string readableAttributes();
      uint32_t baseAddr();
      uint32_t memorySize();
      uint32_t alignment();

    private:
      uint32_t _size;
      int32_t _attributes;
      uint32_t _base_addr;
      uint32_t _memory_size;
      uint32_t _alignment;

}; // moduleInfoSegment


class moduleInfoModule {
    friend moduleInfo;
    public:

        uint32_t id();
        std::string& sdkVersion();
        std::string& version();
        uint16_t type();
        std::string readableType();
        uint16_t flags();
        std::string readableFlags();
        uint32_t moduleStart();
        uint32_t referenceCount();
        uint32_t moduleEnd();
        uint32_t moduleExit();
        std::string& name();
        uint32_t status();
        std::string readableStatus();

        uint32_t fingerprint();

        std::vector<moduleInfoSegment>& segments();

        uint32_t armExidxStart();
        uint32_t armExidxEnd();
        uint32_t armExtabStart();
        uint32_t armExtabEnd();

    private:
        uint32_t _id;
        std::string _sdk_version;
        std::string _version;
        uint16_t _type;
        uint16_t _flags;
        uint32_t _module_start;
        uint32_t _reference_count;
        uint32_t _module_end;
        uint32_t _module_exit;
        std::string _name;
        uint32_t _status;

        uint32_t _fingerprint;

        std::vector<moduleInfoSegment> _segments;

        uint32_t _arm_exidx_start;
        uint32_t _arm_exidx_end;
        uint32_t _arm_extab_start;
        uint32_t _arm_extab_end;

}; // moduleInfoModule


class moduleInfo {
    public:
        moduleInfo(void* buf, uint32_t size);
        std::vector<moduleInfoModule>& modules();

    private:
        std::vector<moduleInfoModule> _modules;
}; // moduleInfo

}; // coredump
}; // vita

#endif