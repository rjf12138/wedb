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
    LockFreeDSHSet tset;
    SValue value;

    for (int i = 0;i < 20; ++i) {
        value.value = i;
        ASSERT_EQ(tset.insert(value), true);
    }

    for (int i = 0; i < 30; ++i) {
        value.value = i;
        std::cout << "i: " << i << " exit: " << (tset.contains(value) ? "true":"false") << std::endl;
    }

    for (int i = 20; i >= 0; --i) {
        value.value = i;
        tset.remove(value);
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