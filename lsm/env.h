#ifndef __WEDB_ENV_H__
#define __WEDB_ENV_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wedb {

std::string path_join(const std::string &dir, const std::string &name);
std::string dir_name(const std::string &path);
std::string base_name(const std::string &path);

bool file_exists(const std::string &path);
bool dir_exists(const std::string &path);
bool file_size(const std::string &path, uint64_t *size);
bool mkdirs(const std::string &path);
bool read_file(const std::string &path, std::string *data);
bool write_file_atomic(const std::string &path, const std::string &data);
bool remove_file(const std::string &path);
bool rename_file(const std::string &from, const std::string &to);
bool sync_dir(const std::string &path);
bool list_dir(const std::string &path, std::vector<std::string> *names);

class WritableFile {
public:
    WritableFile(void);
    ~WritableFile(void);

    bool open(const std::string &path, bool truncate);
    bool append(const std::string &data, bool sync);
    bool sync(void);
    bool close(void);

    bool is_open(void) const { return fd_ >= 0; }
    uint64_t offset(void) const { return offset_; }
    const std::string &path(void) const { return path_; }

private:
    WritableFile(const WritableFile &);
    WritableFile &operator=(const WritableFile &);

private:
    int fd_;
    uint64_t offset_;
    std::string path_;
};

class ReadableFile {
public:
    ReadableFile(void);
    ~ReadableFile(void);

    bool open(const std::string &path);
    bool read(uint64_t offset, size_t size, std::string *data) const;
    bool close(void);

    bool is_open(void) const { return fd_ >= 0; }
    uint64_t size(void) const { return size_; }
    const std::string &path(void) const { return path_; }

private:
    ReadableFile(const ReadableFile &);
    ReadableFile &operator=(const ReadableFile &);

private:
    int fd_;
    uint64_t size_;
    std::string path_;
};

} // namespace wedb

#endif
