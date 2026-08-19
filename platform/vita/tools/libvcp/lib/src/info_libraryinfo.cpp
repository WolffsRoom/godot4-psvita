#include "info_libraryinfo.hpp"

#include <sstream>
#include <iomanip>
#include "memstream.hpp"

namespace vita::coredump {
/*

struct libraryInfo_exported_funcs {
    uint32_t nid;
    uint32_t address;
};

struct libraryInfo_exported_vars {
    uint32_t nid;
    uint32_t address;
};

struct libraryInfo_info {
    uint32_t unk00; // zeroes
    uint32_t id; // 04
    uint32_t module_id; // 08
    uint32_t attr; // 0C
    uint32_t ref_count;  // 10
    uint32_t exported_func_count;  // 14
    uint32_t exported_var_count;  // 18
    uint32_t tls_offsets_count;  // 1C - never encountered non-zero
    uint32_t client_module_id_count;  // 20
    // exported_func_count libraryInfo_exported_funcs follow
    // exported_var_count libraryInfo_exported_vars follow
    // client_module_id_count uint32_t mid follow
    // tls_offsets_count uint32_t offset follow
    // uint32_t name_length;
    // char name[align(name_length, 4)];
};

struct libraryInfoLibrary {
    uint32_t unk00; // coredump minver
    uint32_t count; // records count
    // count of libraryInfo_budget follows
};

*/

    libraryInfo::libraryInfo(void *buf, uint32_t size) {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);

        uint32_t count;
        read_var(count);
        _libraries.resize(count);
        for(uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);
            read_var(_libraries[i]._id);
            read_var(_libraries[i]._module_id);
            read_var(_libraries[i]._attr);
            in.seekg(4, std::ios_base::cur); // ref count, always 0

            uint32_t export_func_count, export_vars_count, modules_count, tls_offset_count;

            read_var(export_func_count);
            _libraries[i]._export_funcs.resize(export_func_count);

            read_var(export_vars_count);
            _libraries[i]._export_vars.resize(export_vars_count);

            read_var(tls_offset_count);
            _libraries[i]._tls_offsets.resize(tls_offset_count);

            read_var(modules_count);
            _libraries[i]._client_module_ids.resize(modules_count);

            for(uint32_t c = 0; c < export_func_count; ++c)
            {
                read_var(_libraries[i]._export_funcs[c].nid);
                read_var(_libraries[i]._export_funcs[c].address);
            }

            for(uint32_t c = 0; c < export_vars_count; ++c)
            {
                read_var(_libraries[i]._export_vars[c].nid);
                read_var(_libraries[i]._export_vars[c].address);
            }

            for(uint32_t c = 0; c < modules_count; ++c)
            {
                read_var(_libraries[i]._client_module_ids[c]);
            }

            for(uint32_t c = 0; c < tls_offset_count; ++c)
            {
                read_var(_libraries[i]._tls_offsets[c]);
            }

            uint32_t path_length;

            read_var(path_length);
            uint32_t path_size = (path_length + 3) & ~0x03; // align to 4
            read_string(_libraries[i]._name, path_size);
        }
    }

    std::vector<libraryInfoLibrary> &libraryInfo::libraries() {
        return _libraries;
    }

    uint32_t libraryInfoLibrary::id() {
        return _id;
    }

    uint32_t libraryInfoLibrary::moduleId() {
        return _module_id;
    }

    uint32_t libraryInfoLibrary::attr() {
        return _attr;
    }

    std::string libraryInfoLibrary::readableAttr() {
        return std::string();
    }

    std::vector<libraryInfoNids>& libraryInfoLibrary::exportedFuncs() {
        return _export_funcs;
    }

    std::vector<libraryInfoNids>& libraryInfoLibrary::exportedVars() {
        return _export_vars;
    }

    std::vector<uint32_t>& libraryInfoLibrary::clientModuleIds() {
        return _client_module_ids;
    }

    std::vector<uint32_t>& libraryInfoLibrary::tlsOffsets() {
        return _tls_offsets;
    }

    std::string &libraryInfoLibrary::name() {
        return _name;
    }

}