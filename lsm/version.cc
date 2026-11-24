#include "version.h"

namespace wedb {

FileMetaData::FileMetaData(void)
: level(0),
  number(0),
  file_size(0),
  max_seq(0)
{
}

Version::Version(void)
{
}

Version::~Version(void)
{
}

void Version::init(int num_levels)
{
    if (num_levels < 1) {
        num_levels = 1;
    }
    levels_.assign(static_cast<size_t>(num_levels), std::vector<FileMetaData>());
}

void Version::clear(void)
{
    for (size_t i = 0; i < levels_.size(); ++i) {
        levels_[i].clear();
    }
}

const std::vector<FileMetaData> &Version::files(int level) const
{
    static const std::vector<FileMetaData> empty;
    if (level < 0 || level >= static_cast<int>(levels_.size())) {
        return empty;
    }
    return levels_[static_cast<size_t>(level)];
}

std::vector<FileMetaData> &Version::files(int level)
{
    static std::vector<FileMetaData> empty;
    if (level < 0 || level >= static_cast<int>(levels_.size())) {
        empty.clear();
        return empty;
    }
    return levels_[static_cast<size_t>(level)];
}

bool Version::add_file(const FileMetaData &file)
{
    if (file.level < 0 || file.level >= static_cast<int>(levels_.size())) {
        return false;
    }
    std::vector<FileMetaData> &level_files = levels_[static_cast<size_t>(file.level)];
    for (size_t i = 0; i < level_files.size(); ++i) {
        if (level_files[i].number == file.number) {
            return false;
        }
    }
    level_files.push_back(file);
    return true;
}

bool Version::remove_file(int level, uint64_t number)
{
    if (level < 0 || level >= static_cast<int>(levels_.size())) {
        return false;
    }
    std::vector<FileMetaData> &level_files = levels_[static_cast<size_t>(level)];
    for (std::vector<FileMetaData>::iterator iter = level_files.begin();
         iter != level_files.end(); ++iter) {
        if (iter->number == number) {
            level_files.erase(iter);
            return true;
        }
    }
    return false;
}

bool Version::has_file(int level, uint64_t number) const
{
    if (level < 0 || level >= static_cast<int>(levels_.size())) {
        return false;
    }
    const std::vector<FileMetaData> &level_files = levels_[static_cast<size_t>(level)];
    for (size_t i = 0; i < level_files.size(); ++i) {
        if (level_files[i].number == number) {
            return true;
        }
    }
    return false;
}

uint64_t Version::level_bytes(int level) const
{
    if (level < 0 || level >= static_cast<int>(levels_.size())) {
        return 0;
    }
    uint64_t bytes = 0;
    const std::vector<FileMetaData> &level_files = levels_[static_cast<size_t>(level)];
    for (size_t i = 0; i < level_files.size(); ++i) {
        bytes += level_files[i].file_size;
    }
    return bytes;
}

} // namespace wedb
