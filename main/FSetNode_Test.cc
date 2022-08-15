#include "dsn_hash_set.h"
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

TEST_F(DSNHashTableTest, FSetNode)
{
    
}


// struct FSetTask {
//     FSetValue val;
//     FSetNode *lfset_ptr;
// };

// void* producer(void* arg)
// {
//     if (arg == nullptr) {
//         return nullptr;
//     }

//     FSetTask *lfset_ptr = static_cast<FSetTask*>(arg);
//     lfset_ptr->lfset_ptr->insert(lfset_ptr->val);
//     delete lfset_ptr;
//     return nullptr;
// }

// TEST_F(DSNHashTableTest, FSetNode)
// {
//     // 用多个线程插入和删除进行测试
//     os::ThreadPool thread_pool;
//     os::ThreadPoolConfig config = thread_pool.get_threadpool_config();
//     config.threads_num = 32;
//     config.max_waiting_task = 2048;
//     thread_pool.set_threadpool_config(config);
//     while (true) {
//         os::ThreadPoolRunningInfo info = thread_pool.get_running_info();
//         if (info.idle_threads_num + info.running_threads_num == static_cast<int>(config.threads_num)) {
//             break;
//         }
//     }

//     FSetNode node;
//     for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
//         FSetTask *task_ptr = new FSetTask;
//         task_ptr->lfset_ptr = &node;
//         task_ptr->val = FSetValue(i);

//         os::Task task;
//         task.work_func = producer;
//         task.thread_arg = task_ptr;
//         thread_pool.add_task(task);
//     }

//     while (thread_pool.get_running_info().waiting_tasks > 0) {
//         os::Time::sleep(50);
//     }

//     // for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
//     //     fprintf(stderr, "vaild: %s, val: %d\n", node.values[i].valid ? "true":"false", node.values[i].value);
//     // }
// }

}
}
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
