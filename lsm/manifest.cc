#include "manifest.h"
#include "coding.h"
#include "env.h"

#include <sstream>

namespace wedb {

bool load_manifest(const std::string &path, int num_levels,
                   Version *version, uint64_t *last_sequence,
                   uint64_t *next_file_number, uint64_t *log_number)
{
    if (version == nullptr || last_sequence == nullptr ||
        next_file_number == nullptr || log_number == nullptr) {
        return false;
    }

    std::string data;
    if (!read_file(path, &data)) {
        return false;
    }

    version->init(num_levels);
    uint64_t last_seq = 0;
    uint64_t next_file = 1;
    uint64_t log_num = 0;
    bool header_seen = false;

    std::istringstream input(data);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.resize(line.size() - 1);
        }
        if (line.empty()) {
            continue;
        }

        std::istringstream fields(line);
        std::string tag;
        fields >> tag;
        if (tag == "wedb_manifest") {
            int version_number = 0;
            fields >> version_number;
            if (version_number != 1) {
                return false;
            }
            header_seen = true;
        } else if (tag == "last_sequence") {
            fields >> last_seq;
        } else if (tag == "next_file_number") {
            fields >> next_file;
        } else if (tag == "log_number") {
            fields >> log_num;
        } else if (tag == "file") {
            FileMetaData file;
            std::string smallest_hex;
            std::string largest_hex;
            fields >> file.level >> file.number >> file.file_size >> file.max_seq
                   >> smallest_hex >> largest_hex;
            if (!fields || file.level < 0 || file.level >= num_levels ||
                !hex_decode(smallest_hex, &file.smallest_key) ||
                !hex_decode(largest_hex, &file.largest_key)) {
                return false;
            }
            if (!version->add_file(file)) {
                return false;
            }
        } else {
            return false;
        }
    }

    if (!header_seen) {
        return false;
    }

    *last_sequence = last_seq;
    *next_file_number = next_file;
    *log_number = log_num;
    return true;
}

bool save_manifest_atomic(const std::string &path, const Version &version,
                          uint64_t last_sequence, uint64_t next_file_number,
                          uint64_t log_number)
{
    std::ostringstream output;
    output << "wedb_manifest 1\n";
    output << "last_sequence " << last_sequence << "\n";
    output << "next_file_number " << next_file_number << "\n";
    output << "log_number " << log_number << "\n";

    for (int level = 0; level < version.num_levels(); ++level) {
        const std::vector<FileMetaData> &files = version.files(level);
        for (size_t i = 0; i < files.size(); ++i) {
            output << "file " << level << " "
                   << files[i].number << " "
                   << files[i].file_size << " "
                   << files[i].max_seq << " "
                   << hex_encode(files[i].smallest_key) << " "
                   << hex_encode(files[i].largest_key) << "\n";
        }
    }

    return write_file_atomic(path, output.str());
}

} // namespace wedb
