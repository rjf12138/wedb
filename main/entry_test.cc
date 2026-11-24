#include "entry.h"

#include "gtest/gtest.h"

#include <string>

namespace {

std::string make_bytes(size_t size, char seed)
{
    std::string value;
    value.resize(size);
    for (size_t i = 0; i < size; ++i) {
        value[i] = static_cast<char>((seed + i) & 0x7F);
    }
    return value;
}

TEST(EntryTest, RoundTripPut)
{
    const std::string key = "hello";
    const std::string value = "world";
    wedb::Entry input(12345, wedb::EntryType_Put, key, value);

    std::string encoded;
    ASSERT_TRUE(input.encode(&encoded, true));

    wedb::Entry output;
    size_t consumed = 0;
    ASSERT_TRUE(output.decode(encoded.data(), encoded.size(), &consumed));
    EXPECT_EQ(consumed, encoded.size());
    EXPECT_EQ(output.seq(), 12345U);
    EXPECT_EQ(output.type(), wedb::EntryType_Put);
    EXPECT_EQ(output.key(), key);
    EXPECT_EQ(output.value(), value);
}

TEST(EntryTest, RoundTripDelete)
{
    wedb::Entry input(99, wedb::EntryType_Delete, "deleted-key", "");
    std::string encoded;
    ASSERT_TRUE(input.encode(&encoded, true));

    wedb::Entry output;
    size_t consumed = 0;
    ASSERT_TRUE(output.decode(encoded.data(), encoded.size(), &consumed));
    EXPECT_EQ(output.seq(), 99U);
    EXPECT_TRUE(output.is_delete());
    EXPECT_EQ(output.key(), "deleted-key");
    EXPECT_TRUE(output.value().empty());
}

TEST(EntryTest, LengthBoundaries)
{
    const size_t key_sizes[] = {1, 2, 255, 256, 65535};
    const size_t value_sizes[] = {0, 1, 255, 256, 65535, 70000};

    for (size_t i = 0; i < sizeof(key_sizes) / sizeof(key_sizes[0]); ++i) {
        for (size_t j = 0; j < sizeof(value_sizes) / sizeof(value_sizes[0]); ++j) {
            const std::string key = make_bytes(key_sizes[i], 'k');
            const std::string value = make_bytes(value_sizes[j], 'v');
            wedb::Entry input(1, wedb::EntryType_Put, key, value);

            std::string encoded;
            ASSERT_TRUE(input.encode(&encoded, false)) << key_sizes[i] << ":" << value_sizes[j];

            wedb::Entry output;
            size_t consumed = 0;
            ASSERT_TRUE(output.decode(encoded.data(), encoded.size(), &consumed));
            EXPECT_EQ(consumed, encoded.size());
            EXPECT_EQ(output.key(), key);
            EXPECT_EQ(output.value(), value);
        }
    }
}

TEST(EntryTest, RejectsTruncatedData)
{
    wedb::Entry input(7, wedb::EntryType_Put, "key", make_bytes(300, 'v'));
    std::string encoded;
    ASSERT_TRUE(input.encode(&encoded, true));

    const size_t cuts[] = {0, 1, 4, 5, 6, encoded.size() / 2, encoded.size() - 5, encoded.size() - 1};
    for (size_t i = 0; i < sizeof(cuts) / sizeof(cuts[0]); ++i) {
        wedb::Entry output;
        size_t consumed = 0;
        EXPECT_FALSE(output.decode(encoded.data(), cuts[i], &consumed));
    }
}

TEST(EntryTest, RejectsCorruptedCrc)
{
    wedb::Entry input(7, wedb::EntryType_Put, "key", "value");
    std::string encoded;
    ASSERT_TRUE(input.encode(&encoded, true));
    ASSERT_GT(encoded.size(), 4);

    encoded[encoded.size() - 1] = static_cast<char>(encoded[encoded.size() - 1] ^ 0x01);

    wedb::Entry output;
    size_t consumed = 0;
    EXPECT_FALSE(output.decode(encoded.data(), encoded.size(), &consumed));
}

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
