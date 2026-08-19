#include "info_fileinfo.hpp"

#include <cstdint>
#include <vector>
#include "utils.h"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct fileInfo_file {
        uint32_t unk_00;
        int32_t fd; // 0x04
        uint32_t attributes;
        uint32_t flags; // 0C
        int32_t pid; // 10

        uint32_t mode;
        uint64_t file_offset;
        uint64_t file_size;

        uint32_t priority; // 18

        uint32_t unk_1C;
        uint32_t unk_20;
        uint32_t unk_24;
        uint32_t unk_28;
        uint32_t unk_2C;
        uint32_t unk_30;
        uint32_t unk_34;

        uint32_t name_length1;
        char resolved_path[align(name_length1, 4)]; // 3C
        uint32_t name_length2;
        char path[align(name_length2, 4)]; // 3C

    };

    struct fileInfo {
        uint32_t unk0; // min. coredump version
        uint32_t count;
        // count fileInfo_file structs follow
    };
*/

    fileInfo::fileInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf,size);
        in.seekg(4, std::ios_base::beg);
        uint32_t count;
        read_var(count);
        _files.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            in.seekg(4, std::ios_base::cur);

            read_var(_files[i]._fd);
            read_var(_files[i]._attributes);
            read_var(_files[i]._flags);
            read_var(_files[i]._pid);
            read_var(_files[i]._mode);

            read_var(_files[i]._file_offset);
            read_var(_files[i]._file_size);
            read_var(_files[i]._priority);


            in.seekg(28, std::ios_base::cur);

            uint32_t path_length;

            read_var(path_length);
            uint32_t path_size = (path_length + 3) & ~0x03; // align to 4
            read_string(_files[i]._resolved_path, path_size);

            read_var(path_length);
            path_size = (path_length + 3) & ~0x03; // align to 4
            read_string(_files[i]._path, path_size);
        }
    }

    std::vector<fileInfoFile>& fileInfo::files()
    {
        return _files;
    }

    uint32_t fileInfoFile::fd()
    {
        return _fd;
    }

    uint32_t fileInfoFile::attributes()
    {
        return _attributes;
    }

    uint32_t fileInfoFile::flags()
    {
        return _flags;
    }

    uint32_t fileInfoFile::pid()
    {
        return _pid;
    }

    uint32_t fileInfoFile::mode()
    {
        return _mode;
    }

    uint32_t fileInfoFile::priority()
    {
        return _priority;
    }

    uint64_t fileInfoFile::fileOffset()
    {
        return _file_offset;
    }

    uint64_t fileInfoFile::fileSize()
    {
        return _file_size;
    }

    std::string& fileInfoFile::path()
    {
        return _path;
    }

    std::string& fileInfoFile::resolvedPath()
    {
        return _resolved_path;
    }



}; // coredump
}; // vita

