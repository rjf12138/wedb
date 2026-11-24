#ifndef __WEDB_TEST_UTIL_H__
#define __WEDB_TEST_UTIL_H__

#include "env.h"

#include <cstdlib>
#include <string>
#include <unistd.h>

namespace wedb_test {

inline std::string temp_path(const std::string &name)
{
    return "/tmp/wedb_" + name + "_" + std::to_string(static_cast<long long>(::getpid()));
}

inline void remove_tree(const std::string &path)
{
    std::string command = "rm -rf '" + path + "'";
    const int result = std::system(command.c_str());
    (void)result;
}

} // namespace wedb_test

#endif
