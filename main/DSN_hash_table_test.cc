#include "DSN_hash_table.h"
#include "gtest/gtest.h"

using namespace basic;

namespace my {
namespace project {
namespace {

class DSNHashTableTest : public ::testing::Test {
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

TEST_F(DSNHashTableTest, DSNHashTableTest_BasicTest)
{
    LockFreeDSHSet<int> tset;
    SValue<int> value;

    int start_num = 20;
    int end_num = 1600;
    for (int i = start_num; i <= end_num; ++i) {
        value.value = i;
        ASSERT_EQ(tset.insert(value), true);
    }
    ASSERT_EQ(tset.size(), end_num - start_num + 1);
    tset.print();

    for (int i = 0; i < end_num; ++i) {
        value.value = i;
        bool ret = tset.contains(value);
        if (i < start_num) {
            ASSERT_EQ(ret, false);
        } else {
            ASSERT_EQ(ret, true);
        }
    }
    // 删除重新调整数据时存在问题
    int remove_start = 1500;
    for (int i = remove_start; i >= 0; --i) {
        value.value = i;
        tset.remove(value);
    }
    ASSERT_EQ(tset.size(), end_num - remove_start);
    tset.print();
}

}
}
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}