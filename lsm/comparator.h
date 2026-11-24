#ifndef __WEDB_COMPARATOR_H__
#define __WEDB_COMPARATOR_H__

#include <string>

namespace wedb {

// 按无符号字节比较，前缀短的排前面。
class BytewiseComparator {
public:
    // std::map 使用：返回 left < right
    bool operator()(const std::string &left, const std::string &right) const
    {
        return compare(left, right) < 0;
    }

    // 三路比较：-1 / 0 / 1
    int compare(const std::string &left, const std::string &right) const
    {
        size_t min_size = left.size() < right.size() ? left.size() : right.size();
        for (size_t i = 0; i < min_size; ++i) {
            unsigned char l = static_cast<unsigned char>(left[i]);
            unsigned char r = static_cast<unsigned char>(right[i]);
            if (l < r) {
                return -1;
            }
            if (l > r) {
                return 1;
            }
        }
        if (left.size() < right.size()) {
            return -1;
        }
        if (left.size() > right.size()) {
            return 1;
        }
        return 0;
    }
};

} // namespace wedb

#endif
