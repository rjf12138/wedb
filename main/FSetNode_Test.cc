#include "dsn_hash_set.h"
#include "gtest/gtest.h"

using namespace basic;

namespace my {
namespace project {
namespace {

class DSHashSetTest : public ::testing::Test {
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

TEST_F(DSHashSetTest, DSHashSetBasicTest)
{
    DSHashSet ds_set;
    int start_value = -99999;
    int max_value = 99999;

    // 测试在完全为空初始状态进行判断是否存在和删除操作
    for (int i = start_value; i < max_value; ++i) {
        ASSERT_EQ(ds_set.exist(i), false); // 测试数据为空的情况
        ASSERT_EQ(ds_set.remove(i), false); // 测试数据为空的情况
    }

    for (int i = start_value; i < max_value; ++i) {
        //LOG_GLOBAL_DEBUG("Insert i = %d", i);
        ASSERT_EQ(ds_set.insert(i), true);
    }
    //ds_set.print();
    for (int i = start_value; i < max_value; ++i) {
        //LOG_GLOBAL_DEBUG("Exist_1 i = %d", i);
        ASSERT_EQ(ds_set.exist(i), true);
    }

    for (int i = start_value; i < max_value; ++i) {
        //LOG_GLOBAL_DEBUG("Remove i = %d", i);
        ASSERT_EQ(ds_set.remove(i), true);
    }
    //ds_set.print();
    for (int i = start_value; i < max_value; ++i) {
        //LOG_GLOBAL_DEBUG("Exist_2 i = %d", i);
        ASSERT_EQ(ds_set.exist(i), false);
    }
}

//测试多线程
struct Producer {
    DSHashSet *ds_set;
    bool *wait_start;
    int low;
    int high;
};

struct Consumer {
    DSHashSet *ds_set;
    bool *wait_start;
    bool *mark;
    int size;
};

void* producer(void* arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    Producer *lfset_ptr = static_cast<Producer*>(arg);
    LOG_GLOBAL_INFO("Start: %d, End: %d", lfset_ptr->low, lfset_ptr->high);
    while (*(lfset_ptr->wait_start) == true) {
        os::Time::sleep(50);;
    }

    for (int i = lfset_ptr->low; i < lfset_ptr->high; ++i) {
        LOG_GLOBAL_INFO("insert: %d", i);
        lfset_ptr->ds_set->insert(i);
    }
    delete lfset_ptr;
    return nullptr;
}

void* consumers(void* arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    Consumer *lfset_ptr = static_cast<Consumer*>(arg);
    while (*(lfset_ptr->wait_start) == true) {
        os::Time::sleep(50);
    }

    while (true) {
        int i = 0;
        for (; i < lfset_ptr->size; ++i) {
            bool ret = lfset_ptr->ds_set->remove(i);
            if (ret == true) {
                LOG_GLOBAL_INFO("remove: %d", i);
                lfset_ptr->mark[i] = true;
            }
        }

        i = 0;
        for (; i < lfset_ptr->size; ++i) {
            if (lfset_ptr->mark[i] == false) {
                break;
            }
        }
        if (i == lfset_ptr->size) {
            break;
        }
    }
    delete lfset_ptr;
    return nullptr;
}

TEST_F(DSHashSetTest, DSHashSetMutilThreadTest)
{
    int num_gap = 30;
    int max_thread = 60;
    int max_range = 2500000;

    // 用多个线程插入和删除进行测试
    os::ThreadPool thread_pool;
    os::ThreadPoolConfig config = thread_pool.get_threadpool_config();
    config.threads_num = max_thread;
    config.max_waiting_task = 4096;
    thread_pool.set_threadpool_config(config);
    while (true) {
        os::ThreadPoolRunningInfo info = thread_pool.get_running_info();
        if (info.idle_threads_num + info.running_threads_num == static_cast<int>(config.threads_num)) {
            LOG_GLOBAL_INFO("thread_num: %d, Idle: %d, Running: %d", config.threads_num, info.idle_threads_num, info.running_threads_num);
            break;
        }
        os::Time::sleep(1000);
    }

    bool *mark = new bool[max_range];
    for (int i = 0; i < max_range; ++i) {
        mark[i] = false;
    }

    bool wait_start = true;
    int start_pos = 0;
    DSHashSet ds_set;
    for (int i = 0; i < num_gap + 1; ++i) {
        // 生产者
        Producer *task_ptr = new Producer;
        task_ptr->ds_set = &ds_set;
        task_ptr->wait_start = &wait_start;
        task_ptr->low = start_pos;
        task_ptr->high = (start_pos + max_range / num_gap > max_range ? max_range : start_pos + max_range / num_gap);
        start_pos += max_range / num_gap;

        os::Task task;
        task.work_func = producer;
        task.thread_arg = task_ptr;
        thread_pool.add_task(task);

        LOG_GLOBAL_INFO("low: %d, high: %d", task_ptr->low, task_ptr->high);
    }

    for (int i = 0; i < max_thread - num_gap - 3; ++i)
    {
        /// 消费者
        Consumer *con_ptr = new Consumer;
        con_ptr->ds_set = &ds_set;
        con_ptr->wait_start = &wait_start;
        con_ptr->mark = mark;
        con_ptr->size = max_range;
        
        os::Task task;
        task.work_func = consumers;
        task.thread_arg = con_ptr;
        thread_pool.add_task(task);
        LOG_GLOBAL_INFO("Consumer: %d", i);
    }

    char ch = '\0';
    while ((ch = getchar()) != 'q') {
        os::Time::sleep(50);
    }
    wait_start = false;

    bool flag = false;
    while (true) {
        int t_num = 0, f_num = 0;
        int i = 0;
        for (; i < max_range; ++i) {
            if (mark[i] == false) {
                ++f_num;
            } else {
                ++t_num;
            }
        }
        LOG_GLOBAL_INFO("true: %d, false: %d", t_num, f_num);
        if (f_num == 0) {
            LOG_GLOBAL_INFO("Mutil thread test exit!!!");
            break;
        }
        os::Time::sleep(1000);
    }
    delete [] mark;
    // for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
    //     fprintf(stderr, "vaild: %s, val: %d\n", node.values[i].valid ? "true":"false", node.values[i].value);
    // }
}

}
}
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
