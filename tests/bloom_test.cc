#include "bloom_filter.h"

#include "gtest/gtest.h"

#include <string>

namespace {

std::string key_of(int i)
{
    return std::string("key-") + std::to_string(i);
}

TEST(BloomFilterTest, NoFalseNegatives)
{
    wedb::BloomFilter filter(10);
    const int count = 10000;
    for (int i = 0; i < count; ++i) {
        filter.add(key_of(i));
    }
    filter.build();

    for (int i = 0; i < count; ++i) {
        EXPECT_TRUE(filter.may_contain(key_of(i))) << i;
    }
}

TEST(BloomFilterTest, SerializeRoundTrip)
{
    wedb::BloomFilter filter(10);
    for (int i = 0; i < 1000; ++i) {
        filter.add(key_of(i));
    }

    std::string encoded;
    ASSERT_TRUE(filter.encode(&encoded));

    wedb::BloomFilter decoded(1);
    size_t consumed = 0;
    ASSERT_TRUE(decoded.decode(encoded.data(), encoded.size(), &consumed));
    EXPECT_EQ(consumed, encoded.size());

    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(decoded.may_contain(key_of(i)));
    }
}

TEST(BloomFilterTest, FalsePositiveRateIsReasonable)
{
    wedb::BloomFilter filter(10);
    for (int i = 0; i < 10000; ++i) {
        filter.add(key_of(i));
    }
    filter.build();

    int false_positive = 0;
    for (int i = 10000; i < 20000; ++i) {
        if (filter.may_contain(key_of(i))) {
            ++false_positive;
        }
    }
    EXPECT_LT(false_positive, 1000);
}

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
