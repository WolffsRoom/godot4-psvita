#ifndef VITA_COREDUMP_FILEINFO_H
#define VITA_COREDUMP_FILEINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace vita
{
namespace coredump
{

class fileInfo;

class fileInfoFile {
    friend fileInfo;
    public:
        uint32_t fd();

        uint32_t attributes();
        uint32_t flags();

        uint32_t pid();

        uint32_t mode();
        uint64_t fileOffset();
        uint64_t fileSize();

        uint32_t priority();

        std::string& resolvedPath();
        std::string& path();

    private:
        uint32_t _fd;
        uint32_t _attributes;
        uint32_t _flags;
        uint32_t _pid;

        uint32_t _mode;
        uint64_t _file_offset;
        uint64_t _file_size;

        uint32_t _priority;

        std::string _resolved_path;
        std::string _path;
}; // fileInfoFile


class fileInfo {
    public:
        fileInfo(void* buf, uint32_t size);
        std::vector<fileInfoFile>& files();

    private:
        std::vector<fileInfoFile> _files;
}; // fileInfo

}; // coredump
}; // vita

#endif