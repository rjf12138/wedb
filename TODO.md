# wedb TODO

> 当前版本：v0.1.0  
> 原则：先正确性，再性能；不做未验证的优化；保持单进程/单写锁的简单模型，除非基准测试证明必须改。

## P0：正确性与稳定性

- [ ] 为 `ds::SkipList` 增加 `lower_bound`，让 `MemTableIterator::seek` 不再从 `begin()` 线性扫描。
- [ ] 增加故障注入测试：WAL 写入前/后、SSTable rename 前后、MANIFEST 提交前后、compaction 提交前后。
- [ ] 全量跑 ASAN/UBSAN，并加入一个 sanitizer 构建脚本。
- [ ] 对 Entry / Block / SSTable / MANIFEST / WAL 做 fuzz；任何输入只能返回错误，不能崩溃。
- [ ] TableReader 的损坏状态传播到 Scan/Compaction，避免损坏文件被静默跳过。
- [ ] 启动时清理 manifest 中不存在的孤儿 `.sst` 文件；保留 `.tmp` 清理。
- [ ] 为 MANIFEST 增加备份/恢复策略（例如 `MANIFEST.bak` 或双份 manifest）。
- [ ] 增加 `close(flush=true)` 选项，区分“只依赖 WAL”和“正常关闭时 flush 成 SSTable”。
- [ ] 明确并测试 `sync=false` / `sync=true` 的崩溃语义（进程崩溃 vs 掉电）。
- [ ] 增加 key/value 大小边界、空 key、超大 value 的 DB 层测试。
- [ ] 增加 compaction 全 tombstone、空输出、重复 key、跨层覆盖的专门测试。

## P1：性能

- [ ] 实现 WriteBatch + group commit，减少 WAL fsync 次数。
- [ ] 后台 flush / compaction 线程；先保持单写者，只把 flush/compaction 移出写路径。
- [ ] 写限流 / 写停顿策略，避免 compaction 跟不上导致 L0 无限增长。
- [ ] LRU Block Cache，缓存热点 data block。
- [ ] Table Cache / 文件句柄缓存，限制 `max_open_files`。
- [ ] Snappy/Zstd 压缩（先做 block 级压缩，保留 block trailer 的 compression type）。
- [ ] SSTable restart points + 前缀压缩，降低 index/data block 体积。
- [ ] 每 block 或按 filter policy 的 BloomFilter，替代每 SSTable 一个 Bloom。
- [ ] Leveled compaction（L0 重叠、L1-Ln 层内不重叠、按 key range 选择重叠文件）。
- [ ] 按 level 容量和 key range 选择 compaction 输入，而不是简单整层合并。
- [ ] 减少拷贝：key/value move、arena 分配、避免 `ByteBuffer` 反复复制。
- [ ] 基准测试：顺序写、随机写、随机读、Scan、compaction 对 P50/P99 的影响。
- [ ] 与 LevelDB/RocksDB 做同机对比，记录差距和瓶颈。

## P2：功能

- [ ] Snapshot / MVCC：读视图、`last_sequence`、compaction 保留活跃快照可见版本。
- [ ] `WriteBatch` 原子多 key 写。
- [ ] `Get` / `Scan` 的 `ReadOptions`（校验开关、缓存开关、快照）。
- [ ] 备份 / checkpoint / repair 工具。
- [ ] 统计信息：读写次数、Bloom 命中率、cache 命中率、每层文件数/字节数、compaction 次数。
- [ ] 更完整的错误码与错误消息；可选引入 `Status`。
- [ ] 配置校验与 TUI/CLI 配置管理。
- [ ] C API / 安装脚本 / pkg-config / CMake package config。
- [ ] 跨平台（至少 Linux；Windows/macOS 另行评估）。
- [ ] 前缀扫描、范围删除（Range Delete）等扩展操作。
- [ ] 列族（Column Family）等高级特性（优先级低）。

## P3：研究与可选

- [ ] 用 wait-free / lock-free 哈希表论文（`doc/Dynamic-Sized-Nonblocking-Hash-Tables.pdf`）实现并发 Block Cache / 内存索引。
- [ ] B+ 树引擎（与 LSM 并列的独立存储引擎，不混入当前代码）。
- [ ] 自定义内存分配器 / arena，降低 MemTable 和 SSTable 构建的内存碎片。
- [ ] 更深入的崩溃一致性验证（kill -9、断电模拟、文件系统错误注入）。
- [ ] 远端共享包仓库 / 跨机器 artifact（当前 PM 只支持本地仓库）。

## 发布与工程化

- [ ] 每个版本更新 `VERSION`、`CHANGELOG.md`、`.pmconfig/config.json` 的 `version`。
- [ ] 发布前执行：`pm build wedb -r`、全部测试、`scripts/release.sh`。
- [ ] 发布 source tag 和 release archive（`dist/wedb-<version>-linux-x86_64.tar.gz`）。
- [ ] 增加 CI（构建 + 测试 + sanitizer + format check）。
- [ ] 增加 `doc/format.md`，把设计文档第 5 节拆成正式格式规范。
- [ ] 保持 `data_structure`、`algorithm`、`basic` 依赖版本固定，并记录在 release notes。

## 明确不做 / 长期不考虑

- 分布式、多机复制、Raft/Paxos。
- SQL、二级索引、join、复杂查询。
- 多进程共享同一个 DB（当前 `LOCK` 独占）。
- 与 LevelDB 文件格式/API 完全兼容。
- 生产级关键数据存储（v1 没有完整的 repair/备份/运维工具）。
- 缓存/队列语义（TTL、淘汰、消费位点等）。
