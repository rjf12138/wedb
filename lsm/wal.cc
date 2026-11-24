#include "wal.h"
#include "coding.h"

namespace wedb {

namespace {
const uint8_t kWalRecordFull = 1;
const size_t kWalHeaderSize = 4 + 4 + 1;
}

std::string log_file_name(uint64_t number)
{
    return std::string("LOG-") + std::to_string(number);
}

bool parse_log_file_name(const std::string &name, uint64_t *number)
{
    if (number == nullptr || name.size() <= 4 || name.compare(0, 4, "LOG-") != 0) {
        return false;
    }
    const std::string digits = name.substr(4);
    if (digits.empty()) {
        return false;
    }
    uint64_t value = 0;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (digits[i] < '0' || digits[i] > '9') {
            return false;
        }
        value = value * 10 + static_cast<uint64_t>(digits[i] - '0');
    }
    *number = value;
    return true;
}

WalWriter::WalWriter(void)
: sync_(false)
{
}

WalWriter::~WalWriter(void)
{
    close();
}

bool WalWriter::open(const std::string &path, bool sync)
{
    sync_ = sync;
    return file_.open(path, true);
}

bool WalWriter::append(const Entry &entry, bool sync)
{
    if (!file_.is_open()) {
        return false;
    }

    std::string payload;
    if (!entry.encode(&payload, false)) {
        return false;
    }

    std::string crc_data;
    crc_data.reserve(4 + 1 + payload.size());
    put_fixed32(&crc_data, static_cast<uint32_t>(payload.size()));
    crc_data.push_back(static_cast<char>(kWalRecordFull));
    crc_data.append(payload);

    std::string record;
    record.reserve(4 + crc_data.size());
    put_fixed32(&record, crc32(crc_data.data(), crc_data.size()));
    record.append(crc_data);

    return file_.append(record, sync || sync_);
}

bool WalWriter::sync(void)
{
    return file_.sync();
}

bool WalWriter::close(void)
{
    return file_.close();
}

bool read_wal(const std::string &path, std::vector<Entry> *entries, bool *truncated)
{
    if (entries == nullptr || truncated == nullptr) {
        return false;
    }
    entries->clear();
    *truncated = false;

    std::string data;
    if (!read_file(path, &data)) {
        return false;
    }

    size_t pos = 0;
    while (pos < data.size()) {
        if (data.size() - pos < kWalHeaderSize) {
            *truncated = true;
            return true;
        }

        const size_t record_start = pos;
        uint32_t expected_crc = 0;
        uint32_t length = 0;
        if (!get_fixed32(data.data(), data.size(), &pos, &expected_crc) ||
            !get_fixed32(data.data(), data.size(), &pos, &length)) {
            *truncated = true;
            return true;
        }
        if (pos >= data.size()) {
            *truncated = true;
            return true;
        }
        uint8_t type = static_cast<uint8_t>(data[pos++]);
        if (type != kWalRecordFull) {
            return false;
        }
        if (length > data.size() - pos) {
            *truncated = true;
            return true;
        }

        uint32_t actual_crc = crc32(data.data() + record_start + 4, 4 + 1 + length);
        if (actual_crc != expected_crc) {
            return false;
        }

        Entry entry;
        size_t consumed = 0;
        if (!entry.decode(data.data() + pos, length, &consumed) || consumed != length) {
            return false;
        }
        entries->push_back(entry);
        pos += length;
    }
    return true;
}

} // namespace wedb
