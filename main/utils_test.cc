#include "bloom_filter.h"
#include "gtest/gtest.h"

namespace my {
namespace project {
namespace {

class BloomFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }
};

TEST_F(BloomFilterTest, BloomFilter_BasicTest)
{
    basic::ByteBuffer buffer("1");
    BloomFilter bloom_filter;
    bloom_filter.print(buffer);
    bloom_filter.clear();

    buffer.clear();
    buffer.write_string("12345");
    bloom_filter.print(buffer);
    bloom_filter.clear();

    buffer.clear();
    buffer.write_string("4567");
    bloom_filter.print(buffer);
    bloom_filter.clear();

    buffer.clear();
    buffer.write_string("-1");
    bloom_filter.print(buffer);
    bloom_filter.clear();

    buffer.clear();
    buffer.write_string("21");
    bloom_filter.print(buffer);
    bloom_filter.clear();

    // 测试空集
    for (int i = -9999; i < 9999; ++i) {
        buffer.clear();
        buffer.write_string(std::to_string(i));
        ASSERT_EQ(bloom_filter.exist(buffer), false);
    }

    // 多个一组测试
    for (int i = 1; i < 256; ++i) {
        int last_start = -99999;
        for (int j = last_start; j < 99999; ) {
            bloom_filter.push(std::to_string(j));
            ++j;
            if (j - last_start == i) {
                for (int k = last_start; k < j; ++k) {
                    ASSERT_EQ(bloom_filter.exist(std::to_string(k)), true);
                }
            }
        }
    }
}

}
}
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}