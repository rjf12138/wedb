# wedb 设计文档：自用级 LevelDB 风格 LSM KV（简化版 v1）

> 状态：v1 已实现（设计稿 v0.3）  
> 日期：2026-11-24  
> 适用仓库：`/home/ruanjian/workspace/project/wedb`  
> 设计原则：单进程、单写锁、同步 flush/compaction；功能完整、实现简洁、正确性优先；不做未验证的优化。
> 关联材料：
> - `doc/currnet_doc.txt`：最早的 LSM 分层/合并设计（历史稿）
> - `doc/develop/技术选型.txt`：LevelDB 风格 SSTable block 布局（参考稿）
> - `doc/leveldb-handbook/`：LevelDB 原理手册
> - `doc/leveldb_note.txt`、`doc/Dynamic-Sized-Nonblocking-Hash-Tables.pdf`：并发哈希表/无锁数据结构笔记（暂不进入主路径）

---

## 0. 结论摘要

**目标**：实现一个单进程、嵌入式、LevelDB 风格的 KV 数据库，支持 `Put / Get / Delete / Scan`、WAL 崩溃恢复、MemTable 落盘、SSTable 查询和自动 compaction。

**可行性**：可以做成一个“能用”的 v1，但前提是把它定义为**自用级嵌入式 KV**，而不是 LevelDB 的全功能复刻。v1 不追求分布式、事务、列族、压缩、快照、并发和后台线程。

**自用版取舍**：单进程、单写锁、同步 flush/compaction；只做完整基础功能（Put/Get/Delete/Scan/WAL/SSTable/Manifest/整层合并）；不做 Block Cache、Snapshot、WriteBatch、压缩、B+ 树、lock-free。代码量和概念数量都要控制，优先保证“不出错”。

**当前阶段**：v1 已实现并通过 entry/bloom/wal/sstable/db 五组测试；代码在 `lsm/`，CLI 在 `main/wedb_main.cc`。早期 `memory_db.*` / `sstable.*` 原型已删除，Bloom/Entry 已重写，MemTable 采用 `std::map`。

**推荐顺序**：

```text
格式层（Entry / Bloom / CRC / Comparator）
        ↓
MemTable + WAL
        ↓
SSTable 读写
        ↓
DB + Manifest + 崩溃恢复
        ↓
Compaction（整层合并）
        ↓
Iterator / Scan
```

> Cache、Benchmark、Snapshot 等属于 v1 之后的优化，不进入当前主线。

**关于论文**：`doc/leveldb_note.txt` 和 `Dynamic-Sized-Nonblocking-Hash-Tables.pdf` 讨论的是 wait-free/lock-free 哈希集合，适合后续做并发缓存或索引优化；它们不是 LSM 的核心算法，不应阻塞 WAL、SSTable、Manifest、Compaction 的实现。

---

## 1. 目标与非目标

### 1.1 v1 目标

- 单进程、单写锁的嵌入式 KV 数据库，目录即数据库；所有操作串行执行。
- 基本功能完整：Put/Get/Delete/Scan/Flush/Close。
- 对外 API：
  - `Put(key, value)`
  - `Get(key, value)`
  - `Delete(key)`
  - `Scan(start_key, end_key)` / 迭代器
  - `Flush()`、`Compact()`、`Close()`
- 写入先写 WAL，再写 MemTable；进程崩溃后可恢复。
- MemTable 达到阈值后冻结并落盘为 SSTable。
- 支持多层 SSTable、BloomFilter、自动 compaction。
- 重启后能从 Manifest + WAL 恢复文件列表和未落盘数据。
- 具备单元测试、随机对比测试、崩溃恢复测试。

### 1.2 v1 非目标

- 分布式、多机复制、Raft/Paxos。
- SQL、二级索引、事务、跨 key 原子性。
- 列族、快照读、MVCC、WriteBatch。
- 压缩算法（Snappy/Zlib）、restart point、前缀压缩。
- 后台 flush/compaction 线程、并行 compaction、写限流、精细化性能调优。
- B+ 树引擎、LRU Block Cache、lock-free 哈希表、Snapshot。
- 与 LevelDB 的文件格式/API 完全兼容。

### 1.3 “能用”的验收定义

v1 做到以下几点即可认为“能用”：

1. 10 万级 KV 的随机 `Put/Get/Delete` 结果与 `std::map` 一致。
2. 进程在任意写入阶段被杀掉，重启后：
   - 已确认 `sync=true` 的写入不丢；
   - 不把半成品 SSTable/Manifest 当成正式文件；
   - 不崩溃、不返回脏数据。
3. 数据量超过 MemTable 阈值后能自动 flush、自动 compaction，读取仍正确。
4. 迭代器按 key 字节序输出，删除的 key 不出现。
5. 单线程写入吞吐和点查延迟在可接受范围内（教学项目不设硬指标）。

---

## 2. 当前实现状态（v1）

### 2.1 已实现

| 模块 | 状态 | 代码 |
|---|---|---|
| 工程管理 | ✅ 已迁移 pm | `.pmconfig/config.json`，C++14 |
| BloomFilter | ✅ 已重写 | 动态 bitset、序列化、零假阴性测试 |
| Entry 编解码 | ✅ 已重写 | `lsm/entry.*`，mark/seq/op/CRC |
| WAL | ✅ 已实现 | `lsm/wal.*`，追加、重放、torn tail |
| MemTable | ✅ 已实现 | `lsm/memtable.*`，`std::map` + seq |
| SSTable | ✅ 已实现 | `lsm/block.*`、`table_builder.*`、`table_reader.*` |
| Manifest/Version | ✅ 已实现 | `lsm/manifest.*`、`version.*` |
| DB API | ✅ 已实现 | `lsm/db.*`，Put/Get/Delete/Scan/Flush/Compact |
| Compaction | ✅ 已实现 | 整层合并、L5 丢弃 tombstone |
| 测试 | ✅ 已补齐 | entry/bloom/wal/sstable/db 五组 gtest |
| CLI | ✅ 已实现 | `main/wedb_main.cc` |

### 2.2 实现选择（与早期设计的差异）

- **MemTable 用 `std::map`（红黑树）**，不依赖 `data_structure` 的 SkipList。  
  原因：`std::map` 行为确定、没有层高/erase 悬空风险，符合“自用、简洁、不出问题”的目标。SkipList 以后可以作为可选替换。
- **Manifest 用行式文本 `MANIFEST`**，不是 JSON。  
  原因：`basic::WeJson` 的数字统一存成 `double`，uint64 文件编号/序号会丢精度；行式文本更简单、可读、无精度问题。
- **单进程、单写锁、同步 flush/compaction**，不引入后台线程。
- **整层合并**：某层文件数达到阈值就把该层全部文件合并成一个输出文件。
- **DB 依赖**：`algorithm`、`basic`、`gtest`；已移除 `data_structure`。
- **目录/文件**：`MANIFEST`、`LOCK`、`LOG-<n>`、`L0..L5/<n>.sst`。

### 2.3 构建与测试

- 构建：`pm build wedb -r` / `pm -r`。
- 测试 target：`entry_test`、`bloom_test`、`wal_test`、`sstable_test`、`db_test`。
- CLI：`./output/debug/bin/wedb <db_dir> put|get|del|scan ...`。
- `CMakeLists.txt` 由 pm 生成，不应手改；构建配置改 `.pmconfig/config.json`。

---

## 3. 与 LevelDB 的对应关系

| LevelDB 概念 | wedb v1 | 说明 |
|---|---|---|
| MemTable | `MemTable` 基于 `std::map` | 活跃内存表（红黑树）；SkipList 可选替换 |
| Immutable MemTable | 同步 flush，不单独保留队列 | 冻结后立即 flush |
| WAL / LOG | `LOG-xxxxxx` | 写入先落 WAL，再进 MemTable |
| SSTable | data/filter/index/footer | 见第 5 节格式 |
| Manifest | `MANIFEST` | 行式文本快照，原子替换 |
| Version | `Version` / `FileMetaData` | 记录每层文件列表和序号 |
| L0 | flush 出来的 SSTable | 文件可能重叠，逐个检查 |
| L1-L5 | 6 个磁盘层中的后 5 层 | v1 采用整层合并 |
| Leveled Compaction | 不做 | v1 只做整层合并 |
| Block Cache | 不做 | v1 先保证正确性 |
| BloomFilter | 每个 SSTable 一个 | v1 先粗粒度，后续按 block 细化 |

### 3.1 与 LevelDB 的主要差异

1. **层数**：LevelDB 是 L0-L6（7 个磁盘层）；wedb v1 采用 doc 的“1 层内存 + 6 层磁盘”，即 MemTable/Immutable + L0-L5。代码中用 `kNumLevels` 常量，后续可改为 7 个磁盘层。
2. **Compaction 策略**：LevelDB 是 leveled compaction；wedb v1 用**整层合并**：某层文件数达到 4，就把该层全部文件合并成**一个**输出文件放到下一层，底层留在本层。比 doc 的“合并最旧 3 个”更简单，也避免部分合并导致 tombstone 复活。读取时每层最多 4 个文件，逐个检查并按 `seq` 选最新。
3. **Manifest**：LevelDB 是 append-only 二进制 VersionEdit；wedb v1 用单个 `MANIFEST`，写临时文件后原子 rename；没有 CURRENT 间接层，减少文件和故障点。
4. **快照/MVCC**：v1 只保留 seq 用于 compaction 判新旧，不实现快照读。
5. **压缩/restart point/Cache**：v1 不做。

---

## 4. 设计决策

### 4.1 命名与层数

- 内存：`MemTable`（活跃）基于 `std::map`；v1 同步 flush，不保留多个 Immutable MemTable。
- 磁盘：`L0` ~ `L5`，共 6 层。
- `kNumLevels = 6`，`kL0 = 0`，`kLastLevel = 5`。
- L0 是 flush 输出层；L1-L5 每层最多保留 4 个文件，层内文件可能重叠（整层合并模型）。
- 读取顺序：MemTable → Immutable → L0 → L1 → ... → L5；同层内检查所有文件，取 seq 最大。
- v1 不做层内 key range 不重叠的 leveled 约束；后续要升级再调整。

### 4.2 Key / Value / 比较器

- key：`std::string`，按字节比较；空 key 在 DB API 层拒绝。
- value：`basic::ByteBuffer`，允许二进制。
- 比较器：`BytewiseComparator`，用 `unsigned char` + `memcmp`，前缀短的排前面；不要直接用 `std::string::operator<` 处理非 ASCII 二进制 key。
- 限制：
  - key 最大 65535 字节（2 字节长度）；
  - value 最大 16MB - 1（3 字节长度）；
  - 单条记录总长度用 4 字节。

### 4.3 序号 seq

- 每次写操作分配单调递增的 `uint64_t seq`，从 1 开始。
- 同 key 多版本时，`seq` 大的是新版本。
- v1 的 `Get` 总是读最新版本；`seq` 主要用于 compaction 判新旧和未来快照。
- `seq` 持久化在 Entry、WAL、Manifest 的 `last_sequence` 中。

### 4.4 操作类型

- `Put`：写入 value。
- `Delete`：写入 tombstone（value 为空，op=Delete）。
- “修改”在 KV 语义里就是一次 `Put`，不再单独设 Modify 标记。
- 操作类型放在 mark 的 2 个 bit 里（0=Put，1=Delete，2/3 保留）。

### 4.5 CRC

- v1 统一用 **CRC32（4 字节）**，用于 Entry、WAL record、SSTable block、Footer、Manifest。
- 读取时默认校验；校验失败返回 `Corruption`，不返回半条数据。
- CRC 覆盖范围必须写进 `doc/format.md`，例如 Entry 的 CRC 覆盖 `mark..value`。
- `algorithm` 库的 `crc32(uint8_t*, uint16_t)` 长度参数是 16 位，单次最多 64KB；SSTable block（4KB）没问题，WAL/Manifest 大记录需要分块更新或新增增量 CRC 接口。

### 4.6 编码与字节序

- v1 用固定宽度小端编码：`uint16/uint32/uint64` 显式按字节写，不直接 `memcpy` 结构体。
- 后续可换 varint 压缩 key/value 长度；格式版本号保证兼容。
- 所有长度字段读取前先做边界检查，避免整数溢出和越界。

### 4.7 BloomFilter

- 动态 bitset：`std::vector<uint64_t>`。
- 默认 `bits_per_key = 10`，哈希个数 `k = 4~6`；用双哈希 `h1 + i*h2` 生成多个位置。
- v1 每个 SSTable 一个 BloomFilter，覆盖该文件所有 user key（包括 tombstone 的 key）。
- BloomFilter 序列化格式：`bits_per_key(4) | k(4) | bit_count(8) | bits | crc32(4)`。
- 读取时 Bloom 只用于“快速否定”；Bloom 说存在仍需查 index/block。
- TableReader 打开时把 index block 和 filter block 读入内存；这是**元数据缓存**，不是通用 Block Cache。否则每次点查都要先读 footer/index/filter，性能不可接受。

### 4.8 Compaction 策略（整层合并）

v1 不照搬 doc 的“合并最旧 3 个”，改成更简单、更不容易出错的整层合并：

- 每层最多保留 4 个文件（可配置）。
- 某层文件数 >= 4 时，把该层**全部文件**作为输入，合并成**一个**输出文件，放到下一层；L5 的输出仍留在 L5。
- 输出按 key 升序、同 key 只保留 seq 最大；合并完成后 manifest 提交，再删除输入文件。
- 读取时每层最多 4 个文件，逐个检查（Bloom 可跳过），取 seq 最大的 Entry；层内文件允许重叠。
- tombstone：只有 L5 整层合并（全部文件参与）时才丢弃；其他层保留。因为整层合并覆盖了该层所有文件，底层丢弃不会复活旧值。
- 可选容量触发：某层总字节超过容量阈值也触发整层合并。
- flush/compaction 后循环检查，直到所有层文件数 < 4；避免一次写入只下沉一层。
- 不做部分合并（只合并最旧 3 个），避免“留下的文件里还有旧值”导致 tombstone 不能丢、逻辑复杂。
- v1 不做 leveled compaction；如果以后数据量变大，再单独实现。

这样做的代价是写放大比 leveled compaction 高，但换来实现简单、规则少、正确性容易验证，符合自用目标。

### 4.9 并发模型（极简）

- v1 只做**单进程、单写锁**：所有 `Put/Get/Delete/Scan/Flush` 串行执行。
- flush/compaction 在写操作内同步完成，不引入后台线程、条件变量和任务队列。
- 不追求并发吞吐；自用场景下，正确性和代码简单优先。
- 如果以后确实需要并发，再单独设计读写锁/后台线程；v1 不预留复杂并发代码。

### 4.10 崩溃一致性

- 写路径：**先 WAL，后 MemTable**。
- WAL 可配置 `sync`；`sync=true` 时每次写后 `fsync`。
- SSTable/Manifest 写入使用“临时文件 → fsync → rename → fsync 目录”。
- Manifest 提交是文件列表变更的原子点：
  - 提交前崩溃：新 SSTable 是孤儿，启动时删除；
  - 提交后崩溃：旧 WAL/旧 SSTable 可能残留，启动时按 manifest 清理。
- 启动时：
  1. 获取 `LOCK`；
  2. 读 `MANIFEST`；
  3. 打开所有有效 SSTable；
  4. 重放未被 manifest 确认的 WAL；
  5. 删除孤儿/临时文件。

### 4.11 API 风格

延续现有代码风格，用 `bool` + `ErrorCode`（`basic::ErrHandle`）而不是引入完整的 `Status` 体系：

```cpp
class DB : public basic::ErrHandle {
public:
    bool open(const std::string &path, const DBOptions &options = DBOptions());
    bool close(void);

    bool put(const std::string &key, const std::string &value, bool sync = false);
    bool put_buffer(const std::string &key, const basic::ByteBuffer &value, bool sync = false);
    bool get(const std::string &key, std::string *value);
    bool get_buffer(const std::string &key, basic::ByteBuffer *value);
    bool remove(const std::string &key, bool sync = false);
    bool scan(const std::string &start, const std::string &end,
              const std::function<bool(const std::string &, const std::string &)> &callback);

    bool flush(void);
    bool compact(void);
};
```

- `DBOptions`：`create_if_missing`、`error_if_exists`、`write_buffer_size`、`sync`、`block_size`、`bits_per_key`、`compaction_trigger`、`num_levels`。
- 后续如果代码量变大，再考虑引入 `Status`；v1 不混用两套错误体系。

### 4.12 测试策略

- 每个模块一个 gtest 可执行目标：
  - `entry_test`、`bloom_test`、`wal_test`、`sstable_test`、`db_test`（compaction 在 `db_test` 中覆盖）。
- 测试入口放在 `main/`，通过 `.pmconfig/config.json` 的 `targets` 配置。
- 随机对比：随机 `Put/Delete` 与 `std::map` 对比，定期重开 DB。
- 崩溃测试：截断 WAL/SSTable/Manifest，验证恢复行为。
- Fuzz：随机字节喂给 `from_block` / SSTable reader，只能返回错误，不能崩溃。
- 建议在 debug 构建临时加 `-fsanitize=address,undefined` 跑测试。

---

## 5. 磁盘格式规范

> 本节是 wedb 的“格式契约”。实现代码、测试、文档必须一致；修改格式必须提升 `format_version`。

### 5.1 基础编码约定

- 所有多字节整数使用**小端序（little-endian）**。
- 固定宽度编码先实现，后续再换 varint：
  - `uint8`：1 字节
  - `uint16`：2 字节 LE
  - `uint32`：4 字节 LE
  - `uint64`：8 字节 LE
- 所有长度字段读取前必须做边界检查：
  - `read_pos + len <= block_end`
  - `len <= max_limit`
- key/value 长度上限：
  - key：`1..65535` 字节（DB API 拒绝空 key）
  - value：`0..16777214` 字节（3 字节长度）
- 字符串/字节串一律按 `unsigned char` 处理，避免 `char` 符号位影响比较和哈希。

### 5.2 Entry 格式（WAL / SSTable 共用）

```text
| mark(1B) | record_len(4B LE) | key_len(1/2B LE) | key |
| seq(8B LE) | value_len(1/2/3B LE) | value |
| [extend_len(1B) | extend_data]? | [crc32(4B LE)]? |
```

`mark` 位定义（bit0 最低位）：

| bit | 含义 |
|---|---|
| 0 | `has_extend`：是否有扩展内容 |
| 1 | `key_len_width`：0=1 字节，1=2 字节 |
| 2-3 | `value_len_width`：1=1 字节，2=2 字节，3=3 字节（0 非法） |
| 4 | `compressed`：v1 必须为 0；为 1 时读取报 `Unsupported` |
| 5 | `has_crc`：Entry 是否自带 CRC32；落盘/WAL 默认 0，由外层 CRC 覆盖 |
| 6-7 | `op`：0=Put，1=Delete，2/3 保留 |

字段说明：

- `record_len`：`key_len + key + seq + value_len + value + extend_len + extend_data` 的总字节数，不含 `mark`、`record_len`、`crc32`。用于快速跳过和边界检查。
- `seq`：写入序号，`uint64`，单调递增。
- `op=Delete` 时 `value_len=0`，value 为空；读取到 tombstone 返回 NotFound。
- `has_extend=1` 时，`extend_len` 为 1 字节，最多 255 字节；v1 不使用，保留。
- `has_crc=1` 时，`crc32` 位于 Entry 末尾，覆盖 `mark + record_len + key_len + key + seq + value_len + value + extend`。
- **写入顺序必须是：先计算 mark 的所有标志位，再序列化**。这是当前 `KeyValueBlock` 最大的 bug。
- 在 SSTable data block 和 WAL 中，Entry 默认 `has_crc=0`，由 block/WAL record 的 CRC 统一校验；独立 Entry 编解码测试可设 `has_crc=1`。

### 5.3 WAL 记录格式

```text
| crc32(4B LE) | length(4B LE) | type(1B) | payload |
```

- `length`：`payload` 的字节数。
- `type`：
  - `1`：完整 Entry（v1 只使用这一种）
  - `2/3/4`：首片/中间片/尾片（为超大记录预留，v1 不实现）
- `payload`：一个完整 Entry 的字节（`has_crc=0`）。
- `crc32` 覆盖 `length + type + payload`。
- 读取规则：
  - 文件剩余字节不足 4+4+1 或不足 `length`：视为尾部撕裂（torn tail），停止重放，保留已恢复的前缀；
  - CRC 不匹配：停止重放并报告 `Corruption`；
  - `type` 未知：报 `Corruption`。
- 每个 MemTable 对应一个 `LOG-<number>`；MemTable 成功 flush 且 Manifest 提交后，删除旧 LOG。
- `sync=true` 时，每次写 WAL 后调用 `fsync(fd)`；`sync=false` 时由 OS 决定，崩溃可能丢失最后一段。

### 5.4 SSTable 文件布局

```text
| data block 0 | ... | data block N-1 | filter block | index block | footer(48B) |
```

- 文件内所有 offset 都是相对文件起始的绝对偏移。
- data block 默认目标 4KiB，按 entry 边界切分；最后一个 block 可以小于 4KiB。
- v1 不做 restart point、不做前缀压缩、不做压缩算法。

#### data block

```text
| Entry 0 | Entry 1 | ... | Entry N-1 | crc32(4B LE) |
```

- Entry 使用 5.2 的格式，`has_crc=0`；block 末尾 CRC 覆盖整个 block 的 Entry 字节。
- 读取时先校验 block CRC，再逐条解析 Entry；任何长度越界立即返回 `Corruption`。
- block 内 key 严格升序；同一 SSTable 内 key 唯一（flush/compaction 输出时已去重）。

#### index block

```text
| count(4B LE) | [last_key_len(2B LE) | last_key | offset(8B LE) | size(4B LE)] * count | crc32(4B LE) |
```

- 每个 data block 一条索引，`last_key` 是该 block 的最后一个 key。
- `offset` 是 data block 在文件中的起始偏移，`size` 是包含 CRC 的 block 总大小。
- 查询时二分查找第一个 `last_key >= target` 的索引项，读取对应 block。
- v1 只做一层 index；超大 SSTable 后续再引入多级 index。
- TableReader 打开时加载 index/filter 到内存；v1 不做通用 block cache，但这两个元数据必须常驻，否则点查性能会非常差。

#### filter block

```text
| bits_per_key(4B LE) | k(4B LE) | bit_count(8B LE) | bits | crc32(4B LE) |
```

- 一个 SSTable 一个 filter block，覆盖该文件所有 user key。
- `bits` 是 `ceil(bit_count/8)` 字节的位图。
- BloomFilter 只做快速否定；返回“可能存在”时必须继续查 index/block。
- 读取时若 filter block 缺失或损坏，可以降级为“不使用 Bloom”（记录 warning），不能影响正确性。

#### footer（固定 48 字节）

```text
| filter_offset(8B LE) | filter_size(8B LE) |
| index_offset(8B LE)  | index_size(8B LE)  |
| magic(8B)            | format_version(4B LE) | footer_crc32(4B LE) |
```

- `magic`：8 字节固定字符串，例如 `WEDB0001`（实现时定稿）。
- `format_version`：v1 = 1。
- `footer_crc32` 覆盖 footer 前 44 字节。
- 读取时先读文件最后 48 字节，校验 magic/version/footer CRC，再读 filter/index。

### 5.5 Manifest / LOCK / 目录布局

```text
<db_dir>/
├── MANIFEST                # 行式文本快照（v1）
├── LOCK                    # flock 独占锁
├── LOG-000001              # 当前 MemTable 的 WAL
├── L0/
│   └── 000004.sst
├── L1/
│   └── 000005.sst
└── ...
```

**Manifest v1（行式文本）核心格式：**

```text
wedb_manifest 1
last_sequence 12345
next_file_number 12
log_number 7
file 0 4 123456 100 68656c6c6f 776f726c64
file 1 5 234567 200 6161 7a7a
```

- `file` 行字段：`level number size max_seq smallest_hex largest_hex`。
- `smallest` / `largest` 用十六进制字符串保存二进制 key，避免二进制污染文本文件。
- `log_number`：当前仍需保留的最早 WAL 编号；更小的 LOG 可以删除。
- `next_file_number`：下一个可用的 SSTable/LOG 编号。
- `last_sequence`：已分配的最大 seq，启动时用于恢复计数器。
- 不使用 JSON：`basic::WeJson` 的数字类型是 `double`，uint64 文件编号/序号会丢精度。

**Manifest 更新流程（原子、单文件）：**

1. 把新快照写入 `MANIFEST.tmp`；
2. `fsync` 文件；
3. `rename` 为 `MANIFEST`；
4. `fsync` 目录。

> v1 用单个 `MANIFEST` 全量快照，不引入 `CURRENT` 间接层。文件很小，flush/compaction 时才重写；原子 rename 保证要么是旧版本、要么是新版本，不会读到半个文件。这样文件少、代码少、故障点少，符合自用目标。

**启动恢复顺序：**

1. `open` + `flock(LOCK, LOCK_EX|LOCK_NB)`；失败说明已有实例。
2. 读 `MANIFEST`；没有则按 `create_if_missing` 决定新建或报错。
3. 解析 manifest；校验 level/file_number/size 等字段。
4. 打开所有 SSTable footer，校验 magic/version；建立 index/filter 句柄。
5. 找到 `log_number` 及之后的 WAL，按编号顺序重放；遇到 torn tail 停止。
6. 用重放后的 MemTable 恢复 `last_sequence`，必要时立即 flush。
7. 删除不在 manifest 中的孤儿 SSTable、`.tmp` 文件、过期 LOG。

### 5.6 文件生命周期与命名

- `file_number`：单调递增，`SSTable` 和 `LOG` 共用编号空间。
- SSTable 命名：`L<level>/<file_number>.sst`（level 也记录在 manifest，目录只是便于人工查看）。
- WAL 命名：`LOG-<file_number>`。
- Manifest 固定命名：`MANIFEST`。
- 写入顺序：临时文件 `<name>.tmp` → fsync → rename → fsync 目录。
- 删除顺序：先提交 manifest 移除文件，再删除物理文件；启动时清理 manifest 中不存在的文件。
- 任何情况下都不得覆盖已存在的 SSTable 文件；同编号文件已存在视为错误。

### 5.7 格式版本与兼容

- `format_version = 1` 写入 footer 和 manifest。
- 读取到未知 version：拒绝打开，返回 `Unsupported`，不要尝试猜测。
- 修改 Entry/SSTable/WAL/Manifest 任一格式，必须提升 `format_version`，并在 `doc/format.md` 记录变更。
- v1 不做自动迁移；格式升级时新建 DB 或写离线转换工具。

---

## 6. 核心流程

### 6.1 写路径（Put / Delete）

```text
DB::put/remove
  → 加写锁
  → 校验 key/value 长度、DB 状态
  → seq = ++last_sequence
  → 构造 Entry{seq, op, key, value}
  → WAL.Append(entry)         // 先写日志
  → 如果 sync: WAL.Sync()
  → MemTable.Insert(key, entry)
  → 如果 MemTable 超阈值：SwitchMemTable()
  → 释放锁
```

- WAL 写入失败：直接返回错误，不修改 MemTable。
- MemTable 内存分配失败：返回错误；WAL 中可能已有一条记录，重启重放时会恢复，语义上是“写入可能成功”，调用方需按错误处理。
- `Delete` 与 `Put` 同路径，只是 `op=Delete`、value 为空。
- v1 不实现 `WriteBatch`；后续加时，一批操作共享一个 WAL record、连续 seq，保证原子性。

### 6.2 读路径（Get）

```text
DB::get
  → 加读锁（v1 可与写共用一把锁）
  → 候选集合 candidates = {}
  → 查活跃 MemTable：命中则记录 (seq, op, value)
  → 查 Immutable MemTable（新到旧）：命中则记录
  → 查 L0 文件（新到旧）：
       Bloom 否定则跳过；否则 index 定位、读 block、找到 key → 记录
  → 查 L1..L5（每层所有可能包含 key 的文件）：
       同上，逐个文件检查并记录候选
  → 在 candidates 中取 seq 最大的 Entry
  → 最大是 Put：返回值
  → 最大是 Delete：返回 NotFound（tombstone 遮蔽旧值）
  → candidates 为空：NotFound
```

- v1 总是读最新版本；跨层从新到旧检查，但**同一层内文件可能重叠，必须检查所有可能包含该 key 的文件，取 `seq` 最大的 Entry**。不能因为第一个文件命中就返回。
- v1 每层最多 4 个文件，逐文件检查成本可控；如果某个文件 Bloom 返回 false 可以跳过，Bloom 返回 true 时读取并记录候选，继续检查同层其他文件，最后按 `seq` 选最新。
- BloomFilter 只用于跳过文件；不能因为 Bloom 返回 false 就认为数据一定不存在（实现正确时这是成立的，但 Bloom 损坏时要降级为全查）。
- 读取到损坏 block：返回 `Corruption`，不要返回部分数据。

### 6.3 MemTable 冻结与 flush

```text
写入导致 active MemTable >= write_buffer_size
  → active 改为 immutable，放入 immutable 队列
  → 创建新的 active MemTable + 新 LOG
  → 同步 flush 最老的 immutable：
       1. 遍历 MemTable（std::map 已按 key 排序）
       2. TableBuilder 写 data blocks / filter / index / footer 到 L0/<number>.sst.tmp
       3. fsync + rename 为 L0/<number>.sst
       4. 更新 manifest：L0 增加该文件、log_number 前进
       5. 删除旧 LOG
       6. 释放 immutable
  → 如果 L0 达到 compaction 条件，触发 compaction
```

- v1 同步 flush（写在 `Put` 返回前），不做后台线程；这是最简单也最容易验证的模型。
- v1 一个 MemTable flush 输出一个 SSTable，不做文件切分；MemTable 阈值默认 5MB，文件不会过大。
- flush 时 MemTable 内 key 唯一（std::map 赋值覆盖），输出 SSTable 不需要再去重。

### 6.4 Compaction

```text
CompactionPicker：
  找到第一个满足条件的 level：
    - 文件数 >= 4，或
    - level_bytes > capacity[level]
  inputs = 该层全部文件
  输出到 level+1（底层则留在本层）

执行：
  1. 为全部输入文件建立 TableIterator，按 key 归并（最小堆）
  2. 相同 key：保留 seq 最大的 Entry
  3. tombstone 处理：
       - L5 整层合并（全部文件参与）：丢弃 tombstone 和所有更旧版本
       - L0-L4：保留 tombstone
  4. 输出 Entry 写入一个 TableBuilder（一个输出文件）
  5. 所有输出文件 fsync + rename
  6. manifest 提交：目标层 add 输出，源层 delete 全部输入
  7. 删除全部输入文件
```

- 归并必须是“新版本优先”：比较 `(user_key asc, seq desc)`；同一 key 只输出最高 seq。
- 输出文件写入失败/崩溃：manifest 未提交，输出是孤儿，启动时删除；输入文件不受影响。
- 删除输入文件必须在 manifest 提交之后；提交前崩溃，输入仍然有效。
- v1 允许层内文件重叠；读取时必须检查同层所有可能包含该 key 的文件，并按 `seq` 选最新（不能按 file_number 第一个命中就返回）。
- 不做 leveled compaction，也不做“只合并最旧 3 个”的部分合并；整层合并规则更少、更容易验证。
- compaction 结束后继续检查各层，直到所有层文件数 < 4，避免级联下沉只做一半。

### 6.5 崩溃恢复

启动流程见 5.5。补充规则：

- **WAL 重放**：
  - 只重放 `log_number` 及之后的 LOG；
  - 逐条读取，遇到 torn tail 停止；
  - 用 Entry 的 seq 恢复 `last_sequence = max(last_sequence, entry.seq)`；
  - 重放完成后，如果 MemTable 非空，可以选择立即 flush 或继续作为 active MemTable。
- **孤儿文件**：
  - manifest 中没有的 `.sst`：删除；
  - `*.tmp`：删除；
  - 编号小于 `log_number` 的 LOG：删除。
- **Manifest 损坏**：
  - `MANIFEST` 解析失败：拒绝打开，避免误删数据；
  - v1 不做自动回退/修复；建议用户备份数据库目录，必要时人工恢复。
- **SSTable 损坏**：
  - footer/index/block CRC 失败：打开失败或该文件标记为不可读；
  - v1 直接返回 `Corruption`，不做自动修复。

### 6.6 Iterator / Scan

- 输入源：active MemTable、所有 immutable MemTable、L0-L5 的所有 SSTable。
- 每个源提供按 key 升序的迭代器；用最小堆做 k 路归并。
- 相同 key：保留 seq 最大者；如果最终是 tombstone，跳过。
- 用户迭代器输出 `(key, value)`，保证：
  - key 严格升序；
  - 不输出已删除 key；
  - 不输出旧版本。
- v1 不支持快照；迭代器创建时记录当前 `last_sequence`，后续可以扩展为快照读。
- 迭代器需要持有相关 MemTable/SSTable 的生命周期引用，避免 compaction 删除文件后悬空。

### 6.7 状态机与并发（v1）

```text
Idle → Flushing → Idle
Idle → Compacting → Idle
任意状态 → Stopping → Closed
```

- v1 用一个 `std::mutex` 保护 DB 元数据和 MemTable；所有操作串行执行。
- flush/compaction 在 `Put`/`Delete` 调用内同步完成；没有后台线程、条件变量和任务队列。
- `close()`：
  1. 设置停止标志，拒绝新写入；
  2. flush 当前 MemTable（可选）；
  3. 关闭 WAL/SSTable/Manifest；
  4. 释放 LOCK。
- v1 不预留复杂并发代码；以后确有需要再单独设计。

### 6.8 错误处理

- 所有磁盘/格式错误都返回明确错误码，不吞掉：
  - `Corruption`：CRC/长度/格式错误；
  - `IOError`：读写/fsync/rename 失败；
  - `NotFound`：key 不存在；
  - `InvalidArgument`：空 key、超长 key/value；
  - `Unsupported`：未知格式版本/压缩算法；
  - `Locked`：LOCK 被占用。
- 写入路径一旦 WAL 成功、MemTable 插入成功，就应向调用方返回成功；后续 flush/compaction 失败通过后台状态/日志暴露，不能让已成功的写入“消失”。
- 读路径遇到损坏文件时，不能返回旧版本或部分值；必须报错。

---

## 7. 模块划分与代码结构

建议把 `lsm/` 拆成以下文件（名字可按习惯调整，但职责要分离）：

```text
lsm/
├── coding.h/.cc          # 小端读写、长度编码、CRC 辅助
├── comparator.h/.cc      # BytewiseComparator
├── entry.h/.cc           # Entry 结构 + 序列化/反序列化（替代 memory_db.*）
├── bloom_filter.h/.cc    # 动态 BloomFilter + 序列化
├── memtable.h/.cc        # MemTable（std::map）、内存统计
├── wal.h/.cc             # LogWriter / LogReader
├── block_builder.h/.cc   # data block 构建
├── block_reader.h/.cc    # data block 解析
├── table_builder.h/.cc   # SSTable 写入（data/filter/index/footer）
├── table_reader.h/.cc    # SSTable 读取、Get、Iterator
├── version.h/.cc         # FileMetaData / Version / 层级
├── manifest.h/.cc        # MANIFEST 读写、原子替换
├── compaction.h/.cc      # CompactionPicker + 归并
├── db.h                  # 对外 API
├── db_impl.h/.cc         # DB 实现、写路径、读路径、flush、recovery
├── iterator.h/.cc        # 归并迭代器
└── env.h/.cc             # 文件/目录/fsync 抽象（POSIX 实现）
```

测试入口：

```text
main/
├── entry_test.cc
├── bloom_test.cc
├── wal_test.cc
├── sstable_test.cc
├── db_test.cc
└── compaction_test.cc
```

`.pmconfig/config.json` 的 `targets` 示例：

```json
"targets": [
  { "name": "wedb", "type": "executable", "source_files": ["main/utils_test.cc"] },
  { "name": "entry_test", "type": "executable", "source_files": ["main/entry_test.cc"] },
  { "name": "bloom_test", "type": "executable", "source_files": ["main/bloom_test.cc"] },
  { "name": "wal_test", "type": "executable", "source_files": ["main/wal_test.cc"] },
  { "name": "sstable_test", "type": "executable", "source_files": ["main/sstable_test.cc"] },
  { "name": "db_test", "type": "executable", "source_files": ["main/db_test.cc"] }
]
```

> PM 的 `source_dirs` 会把 `lsm/*.cc` 编进每个 target；测试 main 放在 `main/`，每个 target 只指定自己的 `source_files`。当 `targets` 多于一个时，`generate_type` 不生效，每个 target 用各自 `type`。

---

## 8. 实现路线图

> 时间是按业余投入估算，重点是“验收标准”，不要为了赶进度跳过测试。

### P0：设计定稿 + 测试骨架（2-3 天）

**任务：**

- 本文件评审并定稿；
- 补一份 `doc/format.md`（可从第 5 节拆出，附十六进制示例）；
- 在 `.pmconfig/config.json` 中加测试 targets；
- 新建 `main/entry_test.cc`、`bloom_test.cc` 等空测试文件，保证能构建。

**验收：**

- `pm build wedb -r` 构建所有 target；
- `pm run entry_test` 能启动 gtest 并执行 0 个测试。

### P1：基础组件修复（约 1 周）

**任务：**

1. 重写 BloomFilter：
   - 拷贝真实 key 字节；
   - 动态 bitset；
   - `serialize/deserialize`；
   - 测试零假阴性和误判率。
2. 重写 Entry 编解码：
   - `mark` 先算后写；
   - 边界检查；
   - CRC 可选；
   - seq/op；
   - 测试所有长度边界、截断、CRC 损坏。
**说明：** v1 的 MemTable 使用 `std::map`，不依赖 `data_structure` 的 SkipList，因此 P1 不包含 SkipList 修复。

**验收：**

- Entry 边界测试全部通过；
- Bloom 10 万 key 零假阴性；
- `std::map` MemTable 随机插入/查询/删除无 ASAN 错误。

### P2：MemTable + WAL（约 1 周）

**任务：**

- `MemTable` 基于 `std::map`，支持 `Insert/Get/Contains`；
- 统计内存占用；
- `WAL` LogWriter/LogReader；
- WAL replay；
- MemTable 冻结/切换（先同步 flush，SSTable 可先用假实现或空文件占位）；
- 测试：写 N 条后重开恢复、WAL 每个字节截断不崩溃。

**验收：**

- WAL 重放后数据与写入序列一致；
- 截断在最坏位置只丢失尾部未完成写入；
- 无内存泄漏/越界。

### P3：SSTable 读写（约 1.5-2 周）

**任务：**

- `coding` / block builder / block reader；
- TableBuilder：data/filter/index/footer；
- TableReader：footer 校验、index 二分、block 读取、Bloom 否定；
- 文件临时名 + fsync + rename；
- 测试：空表/单条/大量数据、缺失 key、CRC 损坏、重新打开、迭代顺序。

**验收：**

- 10 万 KV 写入并全部读回；
- 损坏任意 block 返回 `Corruption` 而不是崩溃；
- 读取不存在 key 不误判（Bloom 不能有假阴性）。

### P4：DB + Manifest + 恢复（约 1 周）

**任务：**

- `DB` API；
- `Version`/`FileMetaData`；
- 单个 `MANIFEST` 原子替换 + LOCK；
- 启动恢复流程；
- flush 接入 SSTable；
- 测试：Put/Get/Delete 随机序列、重启恢复、manifest 损坏、孤儿文件清理。

**验收：**

- 随机操作与 `std::map` 对比一致；
- 任意时刻重启后数据一致；
- 另一个进程打开同一个 DB 返回 `Locked`。

### P5：Compaction + Iterator/Scan（约 1.5-2 周）

**任务：**

- 整层合并：某层文件数 >= 4 或超容量时，合并该层全部文件；
- k 路归并迭代器；
- 同 key 按 seq 去重；
- tombstone：L5 整层合并时丢弃，L0-L4 保留；
- 输出一个 SSTable、manifest 提交、输入删除；
- 用户迭代器/范围扫描；
- 测试：重复 key、跨层覆盖、删除、合并中崩溃、合并后读取、Scan 顺序。

**验收：**

- 连续写入触发多轮 compaction 后读取仍与 `std::map` 一致；
- 每层文件数不超过阈值；
- tombstone 不复活旧值；
- 迭代器按 key 升序、不输出已删除 key；
- 崩溃恢复后无孤儿文件。

### P6（可选，v1 之后）：Cache / Benchmark

**说明：** 不属于 v1 的完成条件；只有 v1 功能稳定、需要通过基准测试找瓶颈时才做。

- （可选）LRU block cache；
- （可选）Benchmark 脚本；
- （可选）性能调优：减少拷贝、批量读、index/filter 缓存。

**总投入**：按业余时间约 6-8 周；如果每天投入更多，可压缩到 1 个月左右。关键不是时间，而是每个阶段的验收标准。

---

## 9. 测试策略

### 9.1 单元测试

- `entry_test`：序列化/反序列化、边界长度、mark 组合、CRC 损坏、截断。
- `bloom_test`：零假阴性、误判率、序列化往返。
- `wal_test`：追加/重放、torn tail、CRC 错误、sync。
- `sstable_test`：构建/读取、index 查找、Bloom、block CRC、footer 校验。
- `db_test`：Put/Get/Delete、重开、恢复、flush。
- `compaction_test`：重复 key、tombstone、层级、崩溃点。

### 9.2 随机对比测试

```text
维护 std::map<string,string> expected
循环 N 次：
  随机 Put/Delete
  随机 Get，与 expected 对比
  每 M 次关闭并重开 DB
最后全量 Scan 对比
```

建议 N = 10 万，M = 1000；后续加到 100 万。

### 9.3 崩溃恢复测试

- 在 WAL 写入前/后、MemTable 插入后、SSTable rename 前后、Manifest 提交前后设置故障点；
- 用截断文件模拟崩溃；
- 验证：
  - 已 `sync` 的写入不丢；
  - 未完成 SSTable/Manifest 不被使用；
  - 重启后无崩溃、无重复/丢失已确认数据。

### 9.4 Fuzz

- 随机字节喂给 `Entry::decode`、`BlockReader`、`TableReader`；
- 只能返回错误，不能越界/崩溃；
- 用 ASAN/UBSAN 跑。

### 9.5 性能测试（可选，v1 之后）

- 顺序写、随机写、随机读、全量扫描；
- 记录吞吐、P50/P99 延迟、SSTable 数量、Bloom 命中率；
- 不设硬指标，但要有基线，便于观察 compaction 是否拖慢写入。

---

## 10. 可行性分析：这样能做出一个“能用的 LevelDB 风格数据库”吗？

### 10.1 结论

**能，但要先把“像 LevelDB”定义清楚。**

- 如果目标是“单进程嵌入式 KV、支持 Put/Get/Delete/Scan、WAL 崩溃恢复、SSTable、自动 compaction、重启不丢数据”，那么按本文路线可以实现一个可用的 v1。
- 如果目标是“和 LevelDB 在性能、并发、压缩、快照、文件格式上全面对齐”，那是一个更大的工程，不建议作为第一版目标。

更准确的定位是：

> **wedb v1 = 教学/自用级 LevelDB 风格 LSM KV。**  
> 架构、文件层次、读写路径与 LevelDB 同构，但 compaction 策略、Manifest、缓存、并发和格式细节是简化版。

### 10.2 为什么这个方案是可行的

1. **核心算法成熟**：MemTable + WAL + SSTable + Manifest + Compaction 是 LevelDB/RocksDB 已验证的架构，`doc/leveldb-handbook` 已经给出各模块的职责和关键流程。
2. **基础组件基本齐了**：
   - v1 用 `std::map` 做 MemTable（简单、无 SkipList 边界风险）；
   - `basic::ByteBuffer` 可做二进制 buffer；
   - `algorithm` 有 MurmurHash、CRC；
   - `system` 里也有 File/Directory/Thread/Mutex，但 `File` 没有 fsync，且会引入 ncurses；v1 用 `lsm/env.h` 包一层 POSIX。
3. **工程链路已通**：pm 能构建、gtest 能跑、依赖能打包/安装。
4. **可以分层验收**：每层都有明确的输入/输出和测试，不需要一次性写完整个数据库。

### 10.3 与 LevelDB 的真实差距

| 维度 | LevelDB | wedb v1 | 影响 |
|---|---|---|---|
| Compaction | Leveled（L0 重叠，L1-L6 不重叠） | 整层合并（每层最多 4 文件） | 写放大更高；但规则简单、正确性容易验证 |
| Manifest | append-only VersionEdit | 单个 `MANIFEST` 快照 | v1 更简单；大库每次 flush/compaction 重写 manifest |
| 快照/MVCC | 支持 Snapshot | 不支持 | v1 只能读最新值 |
| 压缩 | Snappy | 无 | 磁盘占用更大 |
| Block Cache | LRU | 无通用 Block Cache | index/filter 常驻内存；数据 block 每次读盘/走 OS page cache |
| Bloom | 按 block/filter policy | 每个 SSTable 一个 | 大 SSTable 过滤精度较低 |
| 并发 | 多线程 compaction、读写并发 | 单锁、同步 flush/compaction | v1 吞吐低，但正确性容易保证 |
| 文件格式 | LevelDB 自有格式 | 自定义格式 | 不兼容 LevelDB 文件 |
| API | C++/C | 自定义 C++ | 不兼容 LevelDB API |

这些差距不影响“能用”，但决定了 v1 的定位：**能正确存取的 LSM 数据库，不是 LevelDB 的性能替代品。**

### 10.4 论文/笔记在方案中的位置

需要特别说明：

- `doc/leveldb_note.txt` 是 **wait-free 哈希集合** 的论文笔记；
- `doc/Dynamic-Sized-Nonblocking-Hash-Tables.pdf` 是 **动态大小非阻塞哈希表** 论文；
- 它们研究的是并发哈希表/集合的 wait-free/lock-free 算法，适合：
  - 并发 Block Cache；
  - 内存索引；
  - 高并发场景下的元数据表。
- 它们**不是 LSM 的核心算法**，也不提供 WAL/SSTable/Compaction/Manifest 的设计。

因此正确的使用方式是：

1. v1 用 `std::map` 把 LSM 主流程做对；以后可用 SkipList 或并发哈希表替换；
2. 等 SSTable 和基础读写稳定、确实需要并发优化时，再考虑把某个并发哈希表替换成论文里的 wait-free 实现；
3. 不要把“实现 wait-free 哈希表”当成 v1 的阻塞项，否则主线会被拖住。

另外，doc 里的“7 层 + 每层 4 文件 + 合并最旧 3 个”并不是 LevelDB 的 leveled compaction，它更接近 **size-tiered / universal compaction**。v1 进一步简化为“整层合并”：某层文件数达到 4 就把该层全部文件合并，避免部分合并留下的旧值问题。它不是 LevelDB 的算法，但对自用场景足够，而且规则更少、更容易保证正确。

### 10.5 主要风险与应对

| 风险 | 后果 | 应对 |
|---|---|---|
| 格式层 bug | 数据损坏、无法恢复 | P1 重写 Entry/Bloom，边界测试 + Fuzz |
| 崩溃一致性 | 丢数据/半文件 | WAL 先行、临时文件 + fsync + rename、故障注入测试 |
| Compaction 规则错误 | 旧值复活/删除丢失 | seq 判新旧；整层合并；L5 整层合并时丢弃 tombstone；随机对比测试 |
| 并发设计过早 | 死锁/竞态/难调试 | v1 单锁同步，不引入后台线程 |
| 依赖库 bug | 随机崩溃 | v1 用 std::map 规避 SkipList；补 ASAN 和边界测试 |
| 性能不达标 | 大库不可用 | v1 先正确；Cache/Benchmark 放到 v1 之后 |
| 范围蔓延 | 长期做不完 | 冻结 B+ 树、lock-free、压缩、分布式 |
| PM 依赖版本冲突 | 头文件/库版本不一致 | v1 只依赖 algorithm/basic/gtest，已移除 data_structure |

### 10.6 能做到什么规模

- 自用：10 万~100 万 KV、GB 级数据，v1 可以胜任（取决于 value 大小和机器）。
- 再往上（10GB+、高并发、低延迟），需要：
  - leveled compaction；
  - block cache；
  - 压缩；
  - 多线程 compaction / 写限流；
  - 更精细的 manifest/version；
  - 性能基准和调优。
- 因此建议把 v1 的目标定为“正确、可恢复、可测试”；以后确实需要更大规模时，再单独做性能和规模优化。

### 10.7 v1 的成功标准

1. 功能：Put/Get/Delete/Scan 正确。
2. 持久化：重启/崩溃后数据可恢复。
3. Compaction：整层合并自动触发，层级文件数受控，tombstone 正确。
4. 质量：单元测试 + 随机对比 + 崩溃测试 + ASAN 通过。
5. 可维护：格式文档、模块边界、错误码清晰。
6. 性能：单机自用可接受，不要求达到 LevelDB；基准测试可选。

### 10.8 性能预期（量级，需实测）

> 以下是按当前简化设计（单锁、同步 flush/compaction、整层合并、无通用 Block Cache、无压缩）估算的**量级**，不是承诺值；实际取决于 CPU、SSD/HDD、value 大小、key 分布和文件数量。

| 操作 | 预期量级 | 主要瓶颈 |
|---|---|---|
| MemTable Get | 微秒级 | CPU（std::map 查找） |
| SSTable Get（index/filter/block 都在 OS page cache） | 几十~几百微秒 | index 查找 + block 解析 |
| SSTable Get（冷数据，SSD） | 约 0.1~1 ms/文件 | 磁盘随机读；最坏要查 20+ 个文件 |
| SSTable Get（冷数据，HDD） | 毫秒级/文件 | 磁盘寻道；不适合大量随机点查 |
| Write，`sync=false` | 单线程约 1 万~数万条/秒（小 KV） | CPU/内存；flush/compaction 时会停顿 |
| Write，`sync=true` | SSD 约几百~几千条/秒；HDD 几十~几百条/秒 | 每次 fsync 的磁盘延迟 |
| 顺序 Scan | 约几十~几百 MB/s（取决于盘） | 多文件归并 + 顺序 I/O |
| MemTable flush（5MB） | 几十毫秒级 | 写 SSTable + fsync |
| 单次整层合并（4×5MB） | 几百毫秒级 | 读 4 个文件 + 写 1 个文件 + fsync |
| 大文件重写（GB 级） | 秒级甚至更长 | 整层合并会重写整个输出文件 |

**关键性能特征：**

- **写快、读中、compaction 慢**：WAL 顺序追加很快；MemTable 插入 O(log n)；但 flush/compaction 是同步的，会阻塞当前写操作。
- **单锁串行**：多线程调用没有并行收益，吞吐上限受单核和磁盘限制。
- **最坏读放大**：MemTable + Immutable + L0..L5，每层最多 4 个文件，理论上一次 miss 要检查 20 多个文件；Bloom 能跳过大部分，index/filter 常驻内存后，未命中通常只做内存 Bloom 判断。
- **写放大明显**：整层合并会把整层文件重写一遍；数据从 L0 下沉到 L5，可能被重写多次。自用/GB 级可以接受，写密集/大库不适合。
- **单文件增长**：一次 compaction 只输出一个 SSTable，文件会越来越大；compaction 时间随之增长，延迟尖峰会越来越明显。
- **无压缩**：CPU 省了，但磁盘占用和 I/O 变大；value 很大时更明显。
- **恢复**：WAL 重放是顺序读，通常较快；`sync=false` 时崩溃可能丢最后一段写入。

**与 LevelDB 的直观对比：**

- 小数据量、单线程、读多写少：wedb 的简单实现可能“够快”，因为路径短、没有复杂后台调度。
- 大数据量、写密集、多线程、低 P99：LevelDB/RocksDB 明显更快，因为它们有后台 compaction、leveled compaction、block cache、压缩和多线程读。
- 不建议用 wedb 和 LevelDB 做性能对标；定位不同。

### 10.9 适用场景

适合：

- **个人工具/桌面应用**：配置、状态、文档元数据、文件索引；单进程访问，数据量 MB~GB。
- **离线批处理**：批量导入后按 key/范围查询；写入停顿可接受。
- **单进程低 QPS 服务**：内部工具、个人服务、原型系统；能接受 compaction 时的延迟尖峰。
- **日志/事件存储**：key 设计成时间戳/序号，按范围 Scan；写入以追加为主。
- **读多写少的小数据集**：热点数据在 MemTable/OS page cache，读性能足够。
- **学习/实验**：理解 LSM、WAL、SSTable、Manifest、compaction 的完整链路。
- **嵌入式/边缘设备（谨慎）**：单进程、数据量小、可接受 fsync 开销和 flash 写放大；必须做掉电/恢复测试。

### 10.10 不适用场景

不建议：

- **高并发多线程服务**：单锁串行，读会被写/compaction 阻塞。
- **P99 延迟敏感**：同步 flush/compaction 会造成明显停顿。
- **10GB 以上写密集**：整层合并写放大高，单文件重写时间长。
- **多进程共享同一个 DB**：`LOCK` 是独占的，v1 不支持多进程并发访问。
- **事务/多 key 原子性/快照读**：v1 只有单 key Put/Delete。
- **超大 value（>16MB）**：格式上限。
- **生产级关键数据**：v1 没有 repair/自动恢复/备份工具，不建议直接承载不可丢失的数据。
- **缓存/队列**：没有 TTL、淘汰策略、消费位点等语义，不要当 Redis/Kafka 用。
- **关系型/复杂查询**：没有 SQL、二级索引、join；这类场景 SQLite 更合适。

### 10.11 低成本性能优化优先级

在不破坏“简单、单线程”的前提下，优先做这些：

1. **index/filter 常驻内存**（必须做，收益最大）；
2. **默认 `sync=false`**，需要持久化保证时再 `sync=true`；
3. **增大 MemTable 阈值**（例如 16~64MB），减少 flush/compaction 次数；
4. **减少数据拷贝**：key/value 尽量 move，避免 ByteBuffer 反复复制；
5. **Bloom 参数**：`bits_per_key=10~12`，在内存和误判率之间取平衡；
6. **层数按数据量调整**：自用 GB 级可以把磁盘层数从 6 降到 4，减少级联 compaction；
7. **按需再做**：LRU Block Cache、后台 compaction、压缩、leveled compaction。

---

## 11. 待定问题与建议

| 问题 | 选项 | 建议 |
|---|---|---|
| 磁盘层数 | MemTable + L0-L5（6 层） vs L0-L6（7 层） | v1 用 L0-L5，匹配 doc 的“7 层”总述；代码用 `kNumLevels` 常量，后续可改 |
| CRC | CRC16 vs CRC32 | 用 CRC32；CRC16 碰撞率太高，磁盘校验不够 |
| Manifest | 行式文本 vs JSON vs append-only | v1 用行式文本 `MANIFEST`；不做 JSON/append-only |
| Bloom 粒度 | 每 SSTable 一个 vs 每 block 一个 | v1 每 SSTable 一个；大文件多了再细化 |
| tombstone 丢弃 | 永不丢弃 vs 整层合并 | v1 在 L5 整层合并时丢弃；L0-L4 保留 |
| 快照 | v1 是否支持 | 不实现；保留 seq，后续加 Snapshot |
| 压缩 | v1 是否做 | 不做；mark 保留 compressed 位，后续接入 |
| Cache | v1 是否做 | 不做通用 Block Cache；但 TableReader 必须缓存 index/filter 元数据 |
| B+ 树 | 是否现在做 | 不做；LSM v1 完成后再作为第二个引擎 |
| WriteBatch | v1 是否做 | 不做；后续加，能提升吞吐和原子性 |
| 并发 | 单锁 vs 读写锁/后台线程 | v1 单锁同步；不做后台线程 |
| 文件 API | 直接用 POSIX vs `system` 包 | 建议 `lsm/env.h` 包一层 POSIX；`system::File` 没有 fsync，且会引入 ncurses 依赖 |
| 依赖范围 | 是否保留 data_structure | v1 使用 std::map，已移除 data_structure；algorithm/basic/gtest 足够 |

---

## 12. 实施记录与后续

### 12.1 已完成

- [x] 重写 `BloomFilter`：真实 key 拷贝、动态 bitset、序列化、零假阴性测试。
- [x] 重写 `Entry`：mark 先算后写、seq/op、CRC、边界/截断测试。
- [x] 实现 `coding` / `comparator` / `env`（POSIX）。
- [x] 实现 `MemTable`（std::map）+ `WAL` + 重放。
- [x] 实现 `Block` / `TableBuilder` / `TableReader`（data/filter/index/footer）。
- [x] 实现 `Version` / `Manifest`（行式文本、原子替换）。
- [x] 实现 `DB`：Put/Get/Delete/Scan/Flush/Compact、恢复、LOCK。
- [x] 实现整层合并 compaction，L5 丢弃 tombstone。
- [x] 添加 `entry_test` / `bloom_test` / `wal_test` / `sstable_test` / `db_test`。
- [x] 添加 `main/wedb_main.cc` CLI。
- [x] 移除 `data_structure` 依赖，更新 `.pmconfig/config.json` targets。

### 12.2 后续可选

- 将第 5 节拆成独立的 `doc/format.md`，补十六进制示例和格式兼容说明。
- 用 SkipList 替换 std::map（保持 MemTable 接口不变）。
- 增加 LRU Block Cache、后台 compaction、压缩、Snapshot。
- 增加更多崩溃点测试、Fuzz、ASAN/UBSAN 全量跑。
- 数据量变大后再考虑 leveled compaction。

### 12.3 常用命令

```bash
cd /home/ruanjian/workspace/project/wedb

pm -r                         # 增量构建
pm build wedb -r              # 清理后构建
pm deps list wedb             # 查看依赖

./output/debug/bin/entry_test
./output/debug/bin/bloom_test
./output/debug/bin/wal_test
./output/debug/bin/sstable_test
./output/debug/bin/db_test

# CLI
./output/debug/bin/wedb /tmp/wedb_demo put name alice
./output/debug/bin/wedb /tmp/wedb_demo get name
./output/debug/bin/wedb /tmp/wedb_demo scan
./output/debug/bin/wedb /tmp/wedb_demo del name
```

### 12.4 Definition of Done

- 代码能通过 `pm build wedb -r`，并且所有测试目标可构建；
- 新增/修改的逻辑有单元测试；
- 崩溃/边界路径有测试；
- 格式变化同步更新格式文档；
- 不引入未使用的依赖；
- 不把临时调试代码/大文件提交进仓库。

---

## 附录 A：术语

| 术语 | 含义 |
|---|---|
| MemTable | 活跃内存表；v1 用 std::map（红黑树） |
| Immutable MemTable | 已冻结、等待 flush 的内存表 |
| WAL | Write-Ahead Log，预写日志 |
| SSTable | Sorted String Table，有序不可变磁盘表 |
| L0 | flush 输出的磁盘层，文件可能重叠 |
| Compaction | 多层文件归并，去重/删除旧版本 |
| Tombstone | 删除标记，表示 key 已被删除 |
| Manifest | 记录当前版本包含哪些 SSTable 的元数据文件 |
| MANIFEST | 当前文件列表和全局状态的快照 |
| seq | 写入序号，用于判断同一 key 的新旧版本 |
| BloomFilter | 概率型过滤器，用于快速否定 |
| Footer | SSTable 尾部固定长度元数据 |
| Index Block | 记录 data block 位置/最大 key 的索引块 |

## 附录 B：参考资料

- `doc/leveldb-handbook/`：LevelDB 基本概念、读写、日志、MemTable、SSTable、Cache、Bloom、Compaction、Version。
- `doc/develop/技术选型.txt`：LevelDB SSTable block 布局参考。
- `doc/currnet_doc.txt`：最早的 7 层/tiered compaction 设计。
- `doc/leveldb_note.txt`：wait-free 哈希集合论文笔记（后续并发缓存参考）。
- `doc/Dynamic-Sized-Nonblocking-Hash-Tables.pdf`：动态大小非阻塞哈希表论文（后续并发缓存参考）。

## 附录 C：文档维护规则

- 本文档是 wedb 的设计单一来源（single source of truth）。
- `doc/currnet_doc.txt`、`doc/develop/技术选型.txt` 保留为历史材料；与本文冲突时，以本文为准。
- 格式细节以 `doc/format.md` 为准；本文第 5 节是概览。
- 任何格式变更必须：
  1. 提升 `format_version`；
  2. 更新 `doc/format.md`；
  3. 增加兼容/迁移说明；
  4. 补格式测试。

---

> **最终结论**：按本文路线，wedb 可以做成一个正确、可恢复、可测试的自用级 LevelDB 风格 LSM KV。  
> 关键不在“写多少代码”，而在顺序：**先格式与测试，再 WAL/MemTable，再 SSTable，再 DB/Manifest，最后整层合并 + Scan。**  
> Cache、Benchmark、Snapshot、B+ 树、压缩、lock-free 都属于 v1 之后的优化，不要阻塞主线。

