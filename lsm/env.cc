#include "env.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace wedb {

std::string path_join(const std::string &dir, const std::string &name)
{
    if (dir.empty()) {
        return name;
    }
    if (dir[dir.size() - 1] == '/') {
        return dir + name;
    }
    return dir + "/" + name;
}

std::string dir_name(const std::string &path)
{
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return std::string(".");
    }
    if (pos == 0) {
        return std::string("/");
    }
    return path.substr(0, pos);
}

std::string base_name(const std::string &path)
{
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

bool file_exists(const std::string &path)
{
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool dir_exists(const std::string &path)
{
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool file_size(const std::string &path, uint64_t *size)
{
    if (size == nullptr) {
        return false;
    }
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) {
        return false;
    }
    *size = static_cast<uint64_t>(st.st_size);
    return true;
}

bool mkdirs(const std::string &path)
{
    if (path.empty()) {
        return false;
    }
    if (dir_exists(path)) {
        return true;
    }

    std::string current;
    size_t pos = 0;
    if (path[0] == '/') {
        current = "/";
        pos = 1;
    }

    while (pos <= path.size()) {
        size_t next = path.find('/', pos);
        std::string part = path.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
        if (!part.empty()) {
            if (!current.empty() && current[current.size() - 1] != '/') {
                current += "/";
            }
            current += part;
            if (!dir_exists(current)) {
                if (::mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
                    return false;
                }
            }
        }
        if (next == std::string::npos) {
            break;
        }
        pos = next + 1;
    }
    return dir_exists(path);
}

bool read_file(const std::string &path, std::string *data)
{
    if (data == nullptr) {
        return false;
    }
    ReadableFile file;
    if (!file.open(path)) {
        return false;
    }
    data->clear();
    data->resize(static_cast<size_t>(file.size()));
    if (file.size() == 0) {
        return true;
    }
    return file.read(0, static_cast<size_t>(file.size()), data);
}

bool write_file_atomic(const std::string &path, const std::string &data)
{
    const std::string tmp = path + ".tmp";
    WritableFile file;
    if (!file.open(tmp, true)) {
        return false;
    }
    if (!file.append(data, true)) {
        file.close();
        ::unlink(tmp.c_str());
        return false;
    }
    if (!file.sync()) {
        file.close();
        ::unlink(tmp.c_str());
        return false;
    }
    file.close();

    if (::rename(tmp.c_str(), path.c_str()) != 0) {
        ::unlink(tmp.c_str());
        return false;
    }
    return sync_dir(dir_name(path));
}

bool remove_file(const std::string &path)
{
    if (::unlink(path.c_str()) == 0) {
        return true;
    }
    return errno == ENOENT;
}

bool rename_file(const std::string &from, const std::string &to)
{
    return ::rename(from.c_str(), to.c_str()) == 0;
}

bool sync_dir(const std::string &path)
{
    int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        return false;
    }
    bool ok = (::fsync(fd) == 0);
    ::close(fd);
    return ok;
}

bool list_dir(const std::string &path, std::vector<std::string> *names)
{
    if (names == nullptr) {
        return false;
    }
    names->clear();

    DIR *dir = ::opendir(path.c_str());
    if (dir == nullptr) {
        return false;
    }
    struct dirent *entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        names->push_back(name);
    }
    ::closedir(dir);
    return true;
}

WritableFile::WritableFile(void)
: fd_(-1),
  offset_(0)
{
}

WritableFile::~WritableFile(void)
{
    close();
}

bool WritableFile::open(const std::string &path, bool truncate)
{
    close();
    int flags = O_WRONLY | O_CREAT;
    if (truncate) {
        flags |= O_TRUNC;
    } else {
        flags |= O_APPEND;
    }

    fd_ = ::open(path.c_str(), flags, 0644);
    if (fd_ < 0) {
        return false;
    }
    path_ = path;
    if (truncate) {
        offset_ = 0;
    } else {
        uint64_t size = 0;
        if (!file_size(path, &size)) {
            close();
            return false;
        }
        offset_ = size;
    }
    return true;
}

bool WritableFile::append(const std::string &data, bool do_sync)
{
    if (fd_ < 0) {
        return false;
    }

    size_t written = 0;
    while (written < data.size()) {
        ssize_t n = ::write(fd_, data.data() + written, data.size() - written);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        written += static_cast<size_t>(n);
    }
    offset_ += data.size();
    return !do_sync || sync();
}

bool WritableFile::sync(void)
{
    if (fd_ < 0) {
        return false;
    }
    return ::fsync(fd_) == 0;
}

bool WritableFile::close(void)
{
    if (fd_ < 0) {
        return true;
    }
    bool ok = (::close(fd_) == 0);
    fd_ = -1;
    return ok;
}

ReadableFile::ReadableFile(void)
: fd_(-1),
  size_(0)
{
}

ReadableFile::~ReadableFile(void)
{
    close();
}

bool ReadableFile::open(const std::string &path)
{
    close();
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        return false;
    }
    uint64_t size = 0;
    if (!file_size(path, &size)) {
        close();
        return false;
    }
    path_ = path;
    size_ = size;
    return true;
}

bool ReadableFile::read(uint64_t offset, size_t size, std::string *data) const
{
    if (fd_ < 0 || data == nullptr) {
        return false;
    }
    if (offset > size_ || static_cast<uint64_t>(size) > size_ - offset) {
        return false;
    }

    data->resize(size);
    size_t done = 0;
    while (done < size) {
        ssize_t n = ::pread(fd_, &(*data)[done], size - done, static_cast<off_t>(offset + done));
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        done += static_cast<size_t>(n);
    }
    return true;
}

bool ReadableFile::close(void)
{
    if (fd_ < 0) {
        return true;
    }
    bool ok = (::close(fd_) == 0);
    fd_ = -1;
    return ok;
}

} // namespace wedb
