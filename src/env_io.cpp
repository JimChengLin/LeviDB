#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>

#include "env_io.h"
#include "exception.h"

#if defined(OS_MACOSX) || defined(OS_SOLARIS) || defined(OS_FREEBSD) || \
    defined(OS_NETBSD) || defined(OS_OPENBSD) || defined(OS_DRAGONFLYBSD) || \
    defined(OS_ANDROID) || defined(OS_HPUX) || defined(CYGWIN)
#define fread_unlocked fread
#define fwrite_unlocked fwrite
#define fflush_unlocked fflush
#endif

#if defined(OS_MACOSX) || defined(OS_FREEBSD) || \
    defined(OS_OPENBSD) || defined(OS_DRAGONFLYBSD)
#define fdatasync fsync
#endif

#define error_info strerror(errno)

namespace LeviDB {
    namespace IOEnv {
        uint64_t getFileSize(const std::string & fname) {
            struct stat sbuf{};
            if (stat(fname.c_str(), &sbuf) != 0) {
                throw Exception::IOErrorException(fname, error_info);
            }
            return static_cast<uint64_t>(sbuf.st_size);
        };

        bool fileExists(const std::string & fname) noexcept {
            return access(fname.c_str(), F_OK) == 0;
        }

        void deleteFile(const std::string & fname) {
            if (unlink(fname.c_str()) != 0) {
                throw Exception::IOErrorException(fname, error_info);
            }
        }
    }

    FileOpen::FileOpen(const std::string & fname, IOEnv::OpenMode mode) {
        int arg = 0;
        switch (mode) {
            case IOEnv::R_M:
                arg = O_RDONLY;
                break;
            case IOEnv::W_M:
                arg = O_WRONLY | O_CREAT | O_TRUNC;
                break;
            case IOEnv::A_M:
                arg = O_WRONLY | O_CREAT | O_APPEND;
                break;
            case IOEnv::RP_M:
                arg = O_RDWR;
                break;
            case IOEnv::WP_M:
                arg = O_RDWR | O_CREAT | O_TRUNC;
                break;
            case IOEnv::AP_M:
                arg = O_RDWR | O_CREAT | O_APPEND;
                break;
        }

        int fd;
        switch (mode) {
            case IOEnv::W_M:
            case IOEnv::A_M:
            case IOEnv::WP_M:
            case IOEnv::AP_M:
                fd = open(fname.c_str(), arg, 0644/* 权限 */);
                break;
            default:
                fd = open(fname.c_str(), arg);
                break;
        }
        if (fd < 0) {
            throw Exception::IOErrorException(fname, error_info);
        }
        _fd = fd;
    }

    FileFopen::FileFopen(const std::string & fname, IOEnv::OpenMode mode) {
        const char * arg = nullptr;
        switch (mode) {
            case IOEnv::R_M:
                arg = "r";
                break;
            case IOEnv::W_M:
                arg = "w";
                break;
            case IOEnv::A_M:
                arg = "a";
                break;
            case IOEnv::RP_M:
                arg = "r+";
                break;
            case IOEnv::WP_M:
                arg = "w+";
                break;
            case IOEnv::AP_M:
                arg = "a+";
                break;
        }
        FILE * f = fopen(fname.c_str(), arg);
        if (f == nullptr) {
            throw Exception::IOErrorException(fname, error_info);
        }
        _f = f;
    }

    MmapFile::MmapFile(std::string fname)
            : _filename(std::move(fname)),
              _file(_filename, IOEnv::fileExists(_filename) ? IOEnv::RP_M : IOEnv::WP_M),
              _length(IOEnv::getFileSize(_filename)) {
        if (_length == 0) { // 0 长度文件 mmap 会报错
            _length = IOEnv::page_size_;
            if (ftruncate(_file._fd, static_cast<off_t>(_length)) != 0) {
                throw Exception::IOErrorException(_filename, error_info);
            }
        }
        _mmaped_region = mmap(nullptr, _length, PROT_READ | PROT_WRITE, MAP_SHARED, _file._fd, 0);
        if (_mmaped_region == MAP_FAILED) {
            throw Exception::IOErrorException(_filename, error_info);
        }
    }

    void MmapFile::grow() {
        _length += IOEnv::page_size_;
        if (ftruncate(_file._fd, static_cast<off_t>(_length)) != 0) {
            throw Exception::IOErrorException(_filename, error_info);
        }
        // 主要运行在 linux 上, 每次扩容都要 munmap/mmap 的问题可以用 mremap 规避
#ifndef __linux__
        if (munmap(_mmaped_region, _length - IOEnv::page_size_) != 0) {
            throw Exception::IOErrorException(_filename, error_info);
        }
        _mmaped_region = mmap(nullptr, _length, PROT_READ | PROT_WRITE, MAP_SHARED, _file._fd, 0);
#else
        _mmaped_region = mremap(_mmaped_region, _length - IOEnv::page_size_, _length, MREMAP_MAYMOVE);
#endif
        if (_mmaped_region == MAP_FAILED) {
            throw Exception::IOErrorException(_filename, error_info);
        }
    }

    void MmapFile::sync() {
        if (msync(_mmaped_region, _length, MS_SYNC) != 0) {
            throw Exception::IOErrorException(_filename, error_info);
        };
    }

    AppendableFile::AppendableFile(std::string fname)
            : _filename(std::move(fname)), _ffile(_filename, IOEnv::A_M), _length(IOEnv::getFileSize(_filename)) {}

    void AppendableFile::append(const Slice & data) {
        size_t r = fwrite_unlocked(data.data(), 1, data.size(), _ffile._f);
        _length += r;
        if (r != data.size()) {
            throw Exception::IOErrorException(_filename, error_info);
        }
    }

    void AppendableFile::flush() {
        if (fflush_unlocked(_ffile._f) != 0) {
            throw Exception::IOErrorException(_filename, error_info);
        }
    }

    void AppendableFile::sync() {
        flush();
        if (fdatasync(fileno(_ffile._f)) != 0) {
            throw Exception::IOErrorException(_filename, error_info);
        }
    }

    RandomAccessFile::RandomAccessFile(std::string fname)
            : _filename(std::move(fname)), _file(_filename, IOEnv::R_M) {}

    Slice RandomAccessFile::read(uint64_t offset, size_t n, char * scratch) const {
        ssize_t r = pread(_file._fd, scratch, n, static_cast<off_t>(offset));
        if (r < 0) {
            throw Exception::IOErrorException(_filename, error_info);
        }
        return {scratch, static_cast<size_t>(r)};
    }

    SequentialFile::SequentialFile(std::string fname)
            : _filename(std::move(fname)), _ffile(_filename, IOEnv::R_M) {}

    Slice SequentialFile::read(size_t n, char * scratch) {
        size_t r = fread_unlocked(scratch, 1, n, _ffile._f);
        if (r < n) {
            if (static_cast<bool>(feof(_ffile._f))) {
            } else {
                throw Exception::IOErrorException(_filename, error_info);
            }
        }
        return {scratch, r};
    }

    void SequentialFile::skip(uint64_t offset) {
        if (fseek(_ffile._f, static_cast<long>(offset), SEEK_CUR) != 0) {
            throw Exception::IOErrorException(_filename, error_info);
        }
    }

    std::string SequentialFile::readLine() {
        char * line = nullptr;
        size_t len = 0;
        ssize_t read = getline(&line, &len, _ffile._f);

        std::string res;
        if (read != -1) {
            res = {line, static_cast<size_t>(read)};
        }
        free(line);
        return res;
    }
}