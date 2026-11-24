# Changelog

## [0.1.0] - 2026-11-25

首个对外版本（self-use LSM KV）。

### Added

- 嵌入式 KV API：`Put / Get / Delete / Scan / Flush / Compact / Close`。
- WAL 预写日志、重放、torn tail 处理。
- MemTable：基于 `data_structure` 的 `ds::SkipList`（0.1.2）。
- SSTable：data block、index block、BloomFilter、footer。
- 行式文本 `MANIFEST`、`Version`、`FileMetaData`、原子替换与启动恢复。
- 整层合并 compaction；L5 合并时丢弃 tombstone。
- 单进程、单写锁、同步 flush/compaction。
- CLI：`wedb <db_dir> put|get|del|scan`。
- 测试：Entry / Bloom / MemTable / WAL / SSTable / DB。
- 文档：`doc/design.md`、`doc/leveldb-tech-summary.md`、`README.md`、`TODO.md`。

### Fixed

- 修复 `data_structure` SkipList：
  - 头节点层高越界；
  - `random_height` 超出数组范围；
  - `erase()` 返回已释放节点；
  - 节点 `next` 未初始化；
  - 新增 `clear()`。
- 修复 `BlockBuilder` 复写 block 内容（Entry 编码会清空输出 buffer）。
- 修复 `BytewiseComparator` 的 `std::map` bool 比较语义。
- SSTable block 损坏时返回 `Corruption`，不再静默 `NotFound`。
- BloomFilter bit_count 增加上限，避免损坏文件导致超大内存分配。

### Known limitations

- 单进程、单写锁，不支持多进程共享 DB。
- 无 Block Cache / Snapshot / WriteBatch / 压缩。
- 同步 compaction 会造成延迟尖峰。
- 未跑 ASAN/UBSAN、大规模 fuzz 和生产级性能基准。
- 适合自用、工具、离线批处理和小数据量场景。

[0.1.0]: https://github.com/rjf12138/wedb/releases/tag/v0.1.0
