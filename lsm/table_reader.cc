#include "table_reader.h"
#include "block.h"
#include "coding.h"
#include "comparator.h"

#include <cstring>

namespace wedb {

namespace {
const char kTableMagic[9] = "WEDB0001";
const uint32_t kTableFormatVersion = 1;
const size_t kFooterSize = 48;
}

TableIterator::TableIterator(const TableReader *reader)
: reader_(reader),
  block_index_(0),
  pos_(0),
  valid_(false)
{
    if (reader_ != nullptr && !reader_->index().empty()) {
        if (load_block(0)) {
            current_ = entries_[0];
            pos_ = 0;
            valid_ = true;
        }
    }
}

bool TableIterator::load_block(size_t block_index)
{
    if (reader_ == nullptr) {
        return false;
    }
    if (!reader_->read_block(block_index, &entries_) || entries_.empty()) {
        entries_.clear();
        valid_ = false;
        return false;
    }
    block_index_ = block_index;
    pos_ = 0;
    return true;
}

void TableIterator::seek(const std::string &key)
{
    if (reader_ == nullptr || reader_->index().empty()) {
        valid_ = false;
        return;
    }

    BytewiseComparator compare;
    const std::vector<TableReader::IndexEntry> &index = reader_->index();
    size_t left = 0;
    size_t right = index.size();
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (compare.compare(index[mid].last_key, key) < 0) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    if (left >= index.size()) {
        valid_ = false;
        return;
    }

    size_t block = left;
    while (block < index.size()) {
        if (!load_block(block)) {
            valid_ = false;
            return;
        }
        for (size_t i = 0; i < entries_.size(); ++i) {
            if (compare.compare(entries_[i].key(), key) >= 0) {
                pos_ = i;
                current_ = entries_[i];
                valid_ = true;
                return;
            }
        }
        ++block;
    }
    valid_ = false;
}

bool TableIterator::advance_to_next_block(void)
{
    if (reader_ == nullptr) {
        return false;
    }
    size_t next_index = block_index_ + 1;
    if (next_index >= reader_->index().size()) {
        valid_ = false;
        return false;
    }
    if (!load_block(next_index)) {
        valid_ = false;
        return false;
    }
    current_ = entries_[0];
    pos_ = 0;
    valid_ = true;
    return true;
}

void TableIterator::next(void)
{
    if (!valid_) {
        return;
    }
    ++pos_;
    if (pos_ < entries_.size()) {
        current_ = entries_[pos_];
        return;
    }
    if (!advance_to_next_block()) {
        valid_ = false;
    }
}

TableReader::TableReader(void)
: filter_valid_(false),
  corrupted_(false),
  number_(0),
  file_size_(0)
{
}

TableReader::~TableReader(void)
{
}

bool TableReader::open(const std::string &path, uint64_t number)
{
    index_.clear();
    filter_valid_ = false;
    corrupted_ = false;
    number_ = number;
    file_size_ = 0;

    if (!file_.open(path)) {
        return false;
    }
    file_size_ = file_.size();
    if (file_size_ < kFooterSize) {
        return false;
    }

    std::string footer;
    if (!file_.read(file_size_ - kFooterSize, kFooterSize, &footer)) {
        return false;
    }

    size_t pos = 0;
    uint64_t filter_offset = 0;
    uint64_t filter_size = 0;
    uint64_t index_offset = 0;
    uint64_t index_size = 0;
    if (!get_fixed64(footer.data(), footer.size(), &pos, &filter_offset) ||
        !get_fixed64(footer.data(), footer.size(), &pos, &filter_size) ||
        !get_fixed64(footer.data(), footer.size(), &pos, &index_offset) ||
        !get_fixed64(footer.data(), footer.size(), &pos, &index_size)) {
        return false;
    }
    if (pos + 8 > footer.size() || std::memcmp(footer.data() + pos, kTableMagic, 8) != 0) {
        return false;
    }
    pos += 8;
    uint32_t format_version = 0;
    uint32_t footer_crc = 0;
    if (!get_fixed32(footer.data(), footer.size(), &pos, &format_version) ||
        !get_fixed32(footer.data(), footer.size(), &pos, &footer_crc)) {
        return false;
    }
    if (format_version != kTableFormatVersion) {
        return false;
    }
    if (crc32(footer.data(), kFooterSize - 4) != footer_crc) {
        return false;
    }
    if (filter_offset + filter_size > index_offset ||
        index_offset + index_size > file_size_ - kFooterSize ||
        index_size < 4) {
        return false;
    }

    std::string filter_block;
    if (file_.read(filter_offset, static_cast<size_t>(filter_size), &filter_block)) {
        size_t consumed = 0;
        if (filter_.decode(filter_block.data(), filter_block.size(), &consumed) &&
            consumed == filter_block.size()) {
            filter_valid_ = true;
        }
    }

    std::string index_block;
    if (!file_.read(index_offset, static_cast<size_t>(index_size), &index_block)) {
        return false;
    }
    size_t payload_size = index_block.size() - 4;
    size_t crc_pos = payload_size;
    uint32_t expected_crc = 0;
    if (!get_fixed32(index_block.data(), index_block.size(), &crc_pos, &expected_crc) ||
        crc32(index_block.data(), payload_size) != expected_crc) {
        return false;
    }

    size_t read_pos = 0;
    uint32_t count = 0;
    if (!get_fixed32(index_block.data(), payload_size, &read_pos, &count)) {
        return false;
    }
    if (count > payload_size / 14) {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t key_size = 0;
        if (!get_fixed16(index_block.data(), payload_size, &read_pos, &key_size) ||
            key_size > payload_size - read_pos) {
            return false;
        }
        IndexEntry entry;
        entry.last_key.assign(index_block.data() + read_pos, key_size);
        read_pos += key_size;
        if (!get_fixed64(index_block.data(), payload_size, &read_pos, &entry.offset) ||
            !get_fixed32(index_block.data(), payload_size, &read_pos, &entry.size)) {
            return false;
        }
        index_.push_back(entry);
    }
    if (read_pos != payload_size) {
        return false;
    }
    return true;
}

bool TableReader::read_block(size_t block_index, std::vector<Entry> *entries) const
{
    if (entries == nullptr || block_index >= index_.size()) {
        return false;
    }
    const IndexEntry &index = index_[block_index];
    if (index.offset + index.size > file_size_) {
        return false;
    }

    std::string block;
    if (!file_.read(index.offset, index.size, &block)) {
        corrupted_ = true;
        return false;
    }
    BlockReader reader;
    if (!reader.parse(block)) {
        corrupted_ = true;
        return false;
    }
    *entries = reader.entries();
    return true;
}

bool TableReader::get(const std::string &key, Entry *entry) const
{
    if (entry == nullptr) {
        return false;
    }
    if (filter_valid_ && !filter_.may_contain(key)) {
        return false;
    }

    BytewiseComparator compare;
    size_t left = 0;
    size_t right = index_.size();
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (compare.compare(index_[mid].last_key, key) < 0) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    if (left >= index_.size()) {
        return false;
    }

    std::vector<Entry> entries;
    if (!read_block(left, &entries)) {
        return false;
    }
    for (size_t i = 0; i < entries.size(); ++i) {
        int ret = compare.compare(entries[i].key(), key);
        if (ret == 0) {
            *entry = entries[i];
            return true;
        }
        if (ret > 0) {
            return false;
        }
    }
    return false;
}

std::unique_ptr<Iterator> TableReader::new_iterator(void) const
{
    return std::unique_ptr<Iterator>(new TableIterator(this));
}

} // namespace wedb
