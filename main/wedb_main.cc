#include "db.h"

#include <iostream>
#include <string>

namespace {

void usage(const char *program)
{
    std::cout << "usage:" << std::endl;
    std::cout << "  " << program << " <db_dir> put <key> <value>" << std::endl;
    std::cout << "  " << program << " <db_dir> get <key>" << std::endl;
    std::cout << "  " << program << " <db_dir> del <key>" << std::endl;
    std::cout << "  " << program << " <db_dir> scan [start] [end]" << std::endl;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    const std::string path = argv[1];
    const std::string command = argv[2];

    wedb::DB db;
    if (!db.open(path)) {
        std::cerr << "open failed: " << db.last_error_message() << std::endl;
        return 1;
    }

    if (command == "put" && argc >= 5) {
        if (!db.put(argv[3], argv[4])) {
            std::cerr << "put failed: " << db.last_error_message() << std::endl;
            return 1;
        }
        std::cout << "ok" << std::endl;
        return 0;
    }

    if (command == "get" && argc >= 4) {
        std::string value;
        if (!db.get(argv[3], &value)) {
            std::cout << "(not found)" << std::endl;
            return 1;
        }
        std::cout << value << std::endl;
        return 0;
    }

    if (command == "del" && argc >= 4) {
        if (!db.remove(argv[3])) {
            std::cerr << "del failed: " << db.last_error_message() << std::endl;
            return 1;
        }
        std::cout << "ok" << std::endl;
        return 0;
    }

    if (command == "scan") {
        const std::string start = argc >= 4 ? argv[3] : std::string();
        const std::string end = argc >= 5 ? argv[4] : std::string();
        bool ok = db.scan(start, end, [](const std::string &key, const std::string &value) {
            std::cout << key << "\t" << value << std::endl;
            return true;
        });
        if (!ok) {
            std::cerr << "scan failed: " << db.last_error_message() << std::endl;
            return 1;
        }
        return 0;
    }

    usage(argv[0]);
    return 1;
}
