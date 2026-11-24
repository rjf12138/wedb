#include "env.h"
#include "test_util.h"
#include "wal.h"

#include "gtest/gtest.h"

#include <string>
#include <vector>

namespace {

TEST(WalTest, WriteAndReplay)
{
    const std::string path = wedb_test::temp_path("wal") + ".log";
    wedb::remove_file(path);

    wedb::WalWriter writer;
    ASSERT_TRUE(writer.open(path, true));

    const int count = 100;
    for (int i = 0; i < count; ++i) {
        wedb::Entry entry(static_cast<uint64_t>(i + 1), wedb::EntryType_Put,
                          std::string("key-") + std::to_string(i),
                          std::string("value-") + std::to_string(i));
        ASSERT_TRUE(writer.append(entry, false));
    }
    ASSERT_TRUE(writer.close());

    std::vector<wedb::Entry> entries;
    bool truncated = false;
    ASSERT_TRUE(wedb::read_wal(path, &entries, &truncated));
    EXPECT_FALSE(truncated);
    ASSERT_EQ(entries.size(), static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(entries[static_cast<size_t>(i)].seq(), static_cast<uint64_t>(i + 1));
        EXPECT_EQ(entries[static_cast<size_t>(i)].key(), std::string("key-") + std::to_string(i));
        EXPECT_EQ(entries[static_cast<size_t>(i)].value(), std::string("value-") + std::to_string(i));
    }

    wedb::remove_file(path);
}

TEST(WalTest, TruncatedTailDoesNotCrash)
{
    const std::string path = wedb_test::temp_path("wal_truncate") + ".log";
    wedb::remove_file(path);

    wedb::WalWriter writer;
    ASSERT_TRUE(writer.open(path, false));
    for (int i = 0; i < 20; ++i) {
        wedb::Entry entry(static_cast<uint64_t>(i + 1), wedb::EntryType_Put,
                          std::string("key-") + std::to_string(i), "value");
        ASSERT_TRUE(writer.append(entry, false));
    }
    ASSERT_TRUE(writer.close());

    std::string full;
    ASSERT_TRUE(wedb::read_file(path, &full));
    ASSERT_GT(full.size(), 20);

    const size_t cuts[] = {0, 1, 5, full.size() / 2, full.size() - 1};
    for (size_t i = 0; i < sizeof(cuts) / sizeof(cuts[0]); ++i) {
        ASSERT_TRUE(wedb::write_file_atomic(path, full.substr(0, cuts[i])));

        std::vector<wedb::Entry> entries;
        bool truncated = false;
        EXPECT_TRUE(wedb::read_wal(path, &entries, &truncated));
        EXPECT_LE(entries.size(), static_cast<size_t>(20));
    }

    wedb::remove_file(path);
}

TEST(WalTest, CorruptedRecordIsRejected)
{
    const std::string path = wedb_test::temp_path("wal_corrupt") + ".log";
    wedb::remove_file(path);

    wedb::WalWriter writer;
    ASSERT_TRUE(writer.open(path, false));
    wedb::Entry entry(1, wedb::EntryType_Put, "key", "value");
    ASSERT_TRUE(writer.append(entry, false));
    ASSERT_TRUE(writer.close());

    std::string data;
    ASSERT_TRUE(wedb::read_file(path, &data));
    ASSERT_GT(data.size(), 12);
    data[10] = static_cast<char>(data[10] ^ 0x01);
    ASSERT_TRUE(wedb::write_file_atomic(path, data));

    std::vector<wedb::Entry> entries;
    bool truncated = false;
    EXPECT_FALSE(wedb::read_wal(path, &entries, &truncated));

    wedb::remove_file(path);
}

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
