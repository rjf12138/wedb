#include "memtable.h"

#include "gtest/gtest.h"

#include <cstdio>
#include <string>

namespace {

std::string key_of(int i)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "key-%06d", i);
    return std::string(buffer);
}

TEST(MemTableTest, InsertGetIterateAndClear)
{
    wedb::MemTable table;
    const int count = 10000;

    // 逆序插入，验证 SkipList 会按 key 升序组织。
    for (int i = count - 1; i >= 0; --i) {
        wedb::Entry entry(static_cast<uint64_t>(i + 1), wedb::EntryType_Put,
                          key_of(i), std::string("value-") + std::to_string(i));
        ASSERT_TRUE(table.add(entry));
    }

    EXPECT_EQ(table.size(), static_cast<size_t>(count));
    EXPECT_FALSE(table.empty());
    EXPECT_GT(table.approximate_memory_usage(), 0U);
    EXPECT_EQ(table.max_seq(), static_cast<uint64_t>(count));

    for (int i = 0; i < count; ++i) {
        wedb::Entry entry;
        ASSERT_TRUE(table.get(key_of(i), &entry)) << i;
        EXPECT_EQ(entry.key(), key_of(i));
        EXPECT_EQ(entry.value(), std::string("value-") + std::to_string(i));
    }

    std::unique_ptr<wedb::Iterator> iter = table.new_iterator();
    iter->seek("");
    int index = 0;
    std::string previous;
    while (iter->valid()) {
        EXPECT_EQ(iter->entry().key(), key_of(index));
        EXPECT_GT(iter->entry().key(), previous);
        previous = iter->entry().key();
        ++index;
        iter->next();
    }
    EXPECT_EQ(index, count);

    iter = table.new_iterator();
    iter->seek(key_of(5000));
    ASSERT_TRUE(iter->valid());
    EXPECT_EQ(iter->entry().key(), key_of(5000));

    // 同 key 更新：size 不变，value 变为新值。
    wedb::Entry updated(static_cast<uint64_t>(count + 1), wedb::EntryType_Put,
                        key_of(0), "updated-value");
    ASSERT_TRUE(table.add(updated));
    EXPECT_EQ(table.size(), static_cast<size_t>(count));

    wedb::Entry entry;
    ASSERT_TRUE(table.get(key_of(0), &entry));
    EXPECT_EQ(entry.value(), "updated-value");
    EXPECT_EQ(table.max_seq(), static_cast<uint64_t>(count + 1));

    table.clear();
    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0U);
    EXPECT_FALSE(table.get(key_of(0), &entry));
}

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
