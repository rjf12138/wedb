#include "db.h"
#include "error_code.h"
#include "test_util.h"

#include "gtest/gtest.h"

#include <cstdlib>
#include <map>
#include <string>

namespace {

std::string key_of(int i)
{
    return std::string("key-") + std::to_string(1000 + i);
}

wedb::DBOptions test_options(void)
{
    wedb::DBOptions options;
    options.create_if_missing = true;
    options.error_if_exists = false;
    options.write_buffer_size = 2048;
    options.block_size = 128;
    options.bits_per_key = 8;
    options.compaction_trigger = 3;
    options.num_levels = 3;
    return options;
}

void verify_all(wedb::DB *db, const std::map<std::string, std::string> &expected)
{
    for (std::map<std::string, std::string>::const_iterator iter = expected.begin();
         iter != expected.end(); ++iter) {
        std::string value;
        ASSERT_TRUE(db->get(iter->first, &value)) << iter->first;
        EXPECT_EQ(value, iter->second);
    }
}

void verify_scan(wedb::DB *db, const std::map<std::string, std::string> &expected)
{
    std::map<std::string, std::string> actual;
    ASSERT_TRUE(db->scan("", "", [&actual](const std::string &key, const std::string &value) {
        actual[key] = value;
        return true;
    }));

    ASSERT_EQ(actual.size(), expected.size());
    for (std::map<std::string, std::string>::const_iterator iter = expected.begin();
         iter != expected.end(); ++iter) {
        std::map<std::string, std::string>::const_iterator found = actual.find(iter->first);
        ASSERT_TRUE(found != actual.end()) << iter->first;
        EXPECT_EQ(found->second, iter->second);
    }
}

TEST(DBTest, BasicPutGetDeleteReopen)
{
    const std::string dir = wedb_test::temp_path("db_basic");
    wedb_test::remove_tree(dir);

    wedb::DB db;
    ASSERT_TRUE(db.open(dir, test_options())) << db.last_error_message();

    const int count = 100;
    for (int i = 0; i < count; ++i) {
        ASSERT_TRUE(db.put(key_of(i), std::string("value-") + std::to_string(i)))
            << db.last_error_message();
    }

    std::string value;
    ASSERT_TRUE(db.get(key_of(50), &value));
    EXPECT_EQ(value, "value-50");

    for (int i = 0; i < count; i += 10) {
        ASSERT_TRUE(db.remove(key_of(i))) << db.last_error_message();
    }

    ASSERT_TRUE(db.flush()) << db.last_error_message();
    ASSERT_TRUE(db.compact()) << db.last_error_message();
    ASSERT_TRUE(db.close());

    ASSERT_TRUE(db.open(dir, test_options())) << db.last_error_message();
    for (int i = 0; i < count; ++i) {
        const bool deleted = (i % 10) == 0;
        std::string read_value;
        if (deleted) {
            EXPECT_FALSE(db.get(key_of(i), &read_value)) << i;
        } else {
            ASSERT_TRUE(db.get(key_of(i), &read_value)) << i;
            EXPECT_EQ(read_value, std::string("value-") + std::to_string(i));
        }
    }

    std::map<std::string, std::string> expected;
    for (int i = 0; i < count; ++i) {
        if ((i % 10) != 0) {
            expected[key_of(i)] = std::string("value-") + std::to_string(i);
        }
    }
    verify_scan(&db, expected);

    ASSERT_TRUE(db.close());
    wedb_test::remove_tree(dir);
}

TEST(DBTest, RandomCompareWithStdMap)
{
    const std::string dir = wedb_test::temp_path("db_random");
    wedb_test::remove_tree(dir);

    wedb::DBOptions options = test_options();
    wedb::DB db;
    ASSERT_TRUE(db.open(dir, options)) << db.last_error_message();

    std::map<std::string, std::string> expected;
    std::srand(12345);

    const int key_count = 50;
    for (int step = 0; step < 300; ++step) {
        const int key_index = std::rand() % key_count;
        const std::string key = key_of(key_index);
        const int operation = std::rand() % 10;

        if (operation < 5) {
            const std::string new_value = std::string("value-") + std::to_string(step);
            ASSERT_TRUE(db.put(key, new_value)) << db.last_error_message();
            expected[key] = new_value;
        } else if (operation < 7) {
            ASSERT_TRUE(db.remove(key)) << db.last_error_message();
            expected.erase(key);
        } else {
            std::string value;
            std::map<std::string, std::string>::const_iterator iter = expected.find(key);
            if (iter == expected.end()) {
                EXPECT_FALSE(db.get(key, &value));
            } else {
                ASSERT_TRUE(db.get(key, &value)) << db.last_error_message();
                EXPECT_EQ(value, iter->second);
            }
        }

        if ((step % 50) == 49) {
            ASSERT_TRUE(db.close());
            ASSERT_TRUE(db.open(dir, options)) << db.last_error_message();
            verify_all(&db, expected);
        }
    }

    ASSERT_TRUE(db.flush()) << db.last_error_message();
    ASSERT_TRUE(db.compact()) << db.last_error_message();
    verify_all(&db, expected);
    verify_scan(&db, expected);

    ASSERT_TRUE(db.close());
    ASSERT_TRUE(db.open(dir, options)) << db.last_error_message();
    verify_all(&db, expected);

    ASSERT_TRUE(db.close());
    wedb_test::remove_tree(dir);
}

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
