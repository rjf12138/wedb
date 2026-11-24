#ifndef __WEDB_WAL_H__
#define __WEDB_WAL_H__

#include "entry.h"
#include "env.h"

#include <cstdint>
#include <string>
#include <vector>

namespace wedb {

std::string log_file_name(uint64_t number);
bool parse_log_file_name(const std::string &name, uint64_t *number);

class WalWriter {
public:
    WalWriter(void);
    ~WalWriter(void);

    bool open(const std::string &path, bool sync);
    bool append(const Entry &entry, bool sync);
    bool sync(void);
    bool close(void);

    bool is_open(void) const { return file_.is_open(); }
    const std::string &path(void) const { return file_.path(); }

private:
    WalWriter(const WalWriter &);
    WalWriter &operator=(const WalWriter &);

private:
    WritableFile file_;
    bool sync_;
};

// 重放 WAL。torn tail 返回 true 并设置 truncated；格式/CRC 错误返回 false。
bool read_wal(const std::string &path, std::vector<Entry> *entries, bool *truncated);

} // namespace wedb

#endif
