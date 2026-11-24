#ifndef __WEDB_VERSION_H__
#define __WEDB_VERSION_H__

#include <cstdint>
#include <string>
#include <vector>

namespace wedb {

struct FileMetaData {
    int level;
    uint64_t number;
    uint64_t file_size;
    std::string smallest_key;
    std::string largest_key;
    uint64_t max_seq;

    FileMetaData(void);
};

class Version {
public:
    Version(void);
    ~Version(void);

    void init(int num_levels);
    void clear(void);

    int num_levels(void) const { return static_cast<int>(levels_.size()); }
    const std::vector<FileMetaData> &files(int level) const;
    std::vector<FileMetaData> &files(int level);

    bool add_file(const FileMetaData &file);
    bool remove_file(int level, uint64_t number);
    bool has_file(int level, uint64_t number) const;
    uint64_t level_bytes(int level) const;

private:
    std::vector<std::vector<FileMetaData> > levels_;
};

} // namespace wedb

#endif
