#include "env.h"
#include "table_builder.h"
#include "table_reader.h"
#include "test_util.h"

#include "gtest/gtest.h"

#include <string>
#include <vector>

namespace {

std::string key_of(int i)
{
    return std::string("key-") + std::to_string(10000 + i);
}

TEST(SSTableTest, BuildReadAndIterate)
{
    const std::string dir = wedb_test::temp_path("sstable");
    wedb_test::remove_tree(dir);
    ASSERT_TRUE(wedb::mkdirs(dir));

    const std::string path = wedb::path_join(dir, "1.sst");
    wedb::TableBuilderOptions options;
    options.level = 0;
    options.file_number = 1;
    options.block_size = 128;
    options.bits_per_key = 10;

    wedb::TableBuilder builder;
    ASSERT_TRUE(builder.open(path, options));

    const int count = 200;
    for (int i = 0; i < count; ++i) {
        wedb::Entry entry(static_cast<uint64_t>(i + 1), wedb::EntryType_Put,
                          key_of(i), std::string("value-") + std::to_string(i));
        ASSERT_TRUE(builder.add(entry));
    }

    wedb::FileMetaData meta;
    ASSERT_TRUE(builder.finish(&meta));
    EXPECT_EQ(meta.number, 1U);
    EXPECT_EQ(meta.level, 0);
    EXPECT_GT(meta.file_size, 0U);
    EXPECT_EQ(meta.smallest_key, key_of(0));
    EXPECT_EQ(meta.largest_key, key_of(count - 1));

    wedb::TableReader reader;
    ASSERT_TRUE(reader.open(path, 1));
    EXPECT_TRUE(reader.filter_valid());

    for (int i = 0; i < count; ++i) {
        wedb::Entry entry;
        ASSERT_TRUE(reader.get(key_of(i), &entry)) << i;
        EXPECT_EQ(entry.key(), key_of(i));
        EXPECT_EQ(entry.value(), std::string("value-") + std::to_string(i));
    }

    wedb::Entry missing;
    EXPECT_FALSE(reader.get("missing-key", &missing));

    std::unique_ptr<wedb::Iterator> iter = reader.new_iterator();
    iter->seek("");
    int index = 0;
    std::string previous;
    while (iter->valid()) {
        const wedb::Entry &entry = iter->entry();
        EXPECT_EQ(entry.key(), key_of(index));
        EXPECT_GT(entry.key(), previous);
        previous = entry.key();
        ++index;
        iter->next();
    }
    EXPECT_EQ(index, count);

    iter = reader.new_iterator();
    iter->seek(key_of(50));
    ASSERT_TRUE(iter->valid());
    EXPECT_EQ(iter->entry().key(), key_of(50));

    wedb_test::remove_tree(dir);
}

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
