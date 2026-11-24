# wedb

一个自用级、LevelDB 风格的 LSM KV 数据库，使用 C++14 实现。

设计文档见 [`doc/design.md`](doc/design.md)。

## 特性

- `Put / Get / Delete / Scan / Flush / Compact / Close`
- WAL 预写日志 + 崩溃重放
- MemTable（`std::map`）+ SSTable
- data block / index block / BloomFilter / footer
- `MANIFEST` 文件列表与启动恢复
- 整层合并 compaction，L5 丢弃 tombstone
- 单进程、单写锁、同步 flush/compaction
- gtest 单元测试：Entry / Bloom / WAL / SSTable / DB

## 依赖

- C++14、CMake 3.10+
- 通过 pm 管理：`algorithm`、`basic`、`gtest`

## 构建

```bash
cd /home/ruanjian/workspace/project/wedb

pm build wedb -r      # 清理后构建
pm -r                 # 增量构建
```

产物：

```text
output/debug/bin/wedb
output/debug/bin/entry_test
output/debug/bin/bloom_test
output/debug/bin/wal_test
output/debug/bin/sstable_test
output/debug/bin/db_test
```

## CLI

```bash
./output/debug/bin/wedb /tmp/wedb_demo put name alice
./output/debug/bin/wedb /tmp/wedb_demo get name
./output/debug/bin/wedb /tmp/wedb_demo scan
./output/debug/bin/wedb /tmp/wedb_demo del name
```

## C++ API

```cpp
#include "db.h"

wedb::DB db;
wedb::DBOptions options;   // create_if_missing / sync / write_buffer_size / ...
if (!db.open("/tmp/my_db", options)) {
    // db.last_error_message()
}

db.put("key", "value");
std::string value;
db.get("key", &value);
db.remove("key");

db.scan("", "", [](const std::string &key, const std::string &value) {
    // return false 可以提前结束
    return true;
});

db.flush();
db.compact();
db.close();
```

## 数据目录

```text
<db_dir>/
├── MANIFEST       # 行式文本快照
├── LOCK           # flock 独占锁
├── LOG-<n>        # 当前 WAL
├── L0/<n>.sst
├── L1/<n>.sst
└── ...
```

## 测试

```bash
./output/debug/bin/entry_test
./output/debug/bin/bloom_test
./output/debug/bin/wal_test
./output/debug/bin/sstable_test
./output/debug/bin/db_test
```

## 限制

- 单进程、单写锁，不支持多进程共享同一个 DB。
- 无 Block Cache / Snapshot / WriteBatch / 压缩。
- 不支持事务、跨 key 原子性和关系型查询。
- 适合自用、工具、离线批处理和小数据量场景；生产级关键数据请使用 LevelDB/RocksDB。
