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
    //tset.print();

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
    //tset.print();
}



struct FSetTask {
    int start;
    int end;
    LockFreeDSHSet<int> *lfset_ptr;
};

bool start_test_flag = false;
void* producer(void* arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    while (!start_test_flag) {
        os::Time::sleep(3);
    }

    FSetTask *lfset_ptr = static_cast<FSetTask*>(arg);
    for (int i = lfset_ptr->start; i < lfset_ptr->end; ++i) {
        SValue<int> value;
        value.value = i;
        lfset_ptr->lfset_ptr->insert(value);
        //fprintf(stderr, "start: %d, end: %d, total: %d\n", lfset_ptr->start, lfset_ptr->end, lfset_ptr->lfset_ptr->size());
        //os::Time::sleep(5);
    }
    fprintf(stderr, "start: %d, end: %d, total: %d\n", lfset_ptr->start, lfset_ptr->end, lfset_ptr->lfset_ptr->size());
    delete lfset_ptr;
    return nullptr;
}

TEST_F(DSNHashTableTest, DSNHashTableTest_MutiThreadTest)
{
    // 用多个线程插入和删除进行测试
    os::ThreadPool thread_pool;
    os::ThreadPoolConfig config = thread_pool.get_threadpool_config();
    config.threads_num = 7;
    thread_pool.set_threadpool_config(config);
    while (true) {
        os::ThreadPoolRunningInfo info = thread_pool.get_running_info();
        if (info.idle_threads_num + info.running_threads_num == config.threads_num) {
            break;
        }
    }

    LockFreeDSHSet<int> lfset;
    for (int i = 0; i < config.threads_num; ++i) {
        FSetTask *ftask = new FSetTask;
        ftask->start = i * 100;
        ftask->end = ftask->start + 100;
        ftask->lfset_ptr = &lfset;

        os::Task ttask;
        ttask.work_func = producer;
        ttask.thread_arg = ftask;
        thread_pool.add_task(ttask);
    }
    start_test_flag = true;

    while (getchar() != 'q') {
        os::Time::sleep(5);
    }
    fprintf(stderr, "total: %d\n", lfset.size());
}

}
}
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}