#ifndef VITA_COREDUMP_LIBRARYINFO_H
#define VITA_COREDUMP_LIBRARYINFO_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace vita::coredump {

typedef struct {
    uint32_t nid;
    uint32_t address;
} libraryInfoNids;

class libraryInfo;

class libraryInfoLibrary {
    friend libraryInfo;
public:
    uint32_t id();
    uint32_t moduleId();
    uint32_t attr();
    std::string readableAttr();
    std::vector<libraryInfoNids>& exportedFuncs();
    std::vector<libraryInfoNids>& exportedVars();
    std::vector<uint32_t>& clientModuleIds();
    std::vector<uint32_t>& tlsOffsets();
    std::string& name();

private:
    uint32_t _id;
    uint32_t _module_id;
    uint32_t _attr;
    std::string _name;
    std::vector<libraryInfoNids> _export_funcs;
    std::vector<libraryInfoNids> _export_vars;
    std::vector<uint32_t> _client_module_ids;
    std::vector<uint32_t> _tls_offsets;
};

class libraryInfo {
public:
    libraryInfo(void* buf, uint32_t size);
    std::vector<libraryInfoLibrary>& libraries();
private:
    std::vector<libraryInfoLibrary> _libraries;
};

}

#endif //VITA_COREDUMP_LIBRARYINFO_H
