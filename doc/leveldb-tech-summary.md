# LevelDB 技术总结：从一个 KV 请求看 LSM 的全部子系统

> 本文是 wedb 的 LevelDB 技术参考，目标是把 LevelDB 涉及的主要技术点串成一张完整的图。
> 参考材料：`doc/leveldb-handbook/`、LevelDB 源码与公开设计文档。
> 与 wedb v0.1.0 的对照见第 18 节。

## 0. 一句话总结

LevelDB 不是一个“简单的 KV 数据结构”，而是由以下子系统组成的嵌入式存储引擎：

```text
WriteBatch / Snapshot
        │
        ▼
   WAL（预写日志）
        │
        ▼
 MemTable（SkipList / 内存表）
        │  flush
        ▼
 SSTable（data block / filter block / index block / footer）
        │
        ▼
 Manifest / CURRENT / Version（文件版本与恢复）
        │
        ▼
 Compaction（L0-L6 层级合并）
        │
        ▼
 Block Cache / Table Cache / BloomFilter / Iterator
```

本文按“写路径 → 读路径 → 存储格式 → 后台机制 → 工程细节”展开。

---

## 1. 总体架构

```text
                 ┌──────────────┐
                 │   DB::Put    │
                 └──────┬───────┘
                        │
              ┌─────────▼─────────┐
              │      WriteBatch   │  可选：批量写原子性
              └─────────┬─────────┘
                        │
              ┌─────────▼─────────┐
              │      WAL (LOG)    │  顺序追加 + 可选 fsync
              └─────────┬─────────┘
                        │
              ┌─────────▼─────────┐
              │     MemTable      │  SkipList，按 InternalKey 排序
              └─────────┬─────────┘
                        │ 达到 write_buffer_size
              ┌─────────▼─────────┐
              │ Immutable MemTable│  冻结，等待 flush
              └─────────┬─────────┘
                        │
              ┌─────────▼─────────┐
              │  L0 SSTable 文件  │  文件之间 key range 可能重叠
              └─────────┬─────────┘
                        │ compaction
        ┌───────────────▼────────────────┐
        │ L1 / L2 / ... / L6 SSTable 层  │  层内 key range 不重叠
        └───────────────┬────────────────┘
                        │
              ┌─────────▼─────────┐
              │ Manifest / CURRENT│  记录每层有哪些文件、日志号、序列号
              └───────────────────┘
```

关键点：

- **写**只碰 WAL + MemTable，顺序写盘，速度快。
- **读**从 MemTable 开始，逐层向下查 SSTable。
- **flush** 把 MemTable 变成 L0 SSTable。
- **compaction** 把 L0 向 L1-L6 合并，控制读放大和空间放大。
- **Manifest** 保证重启后知道哪些文件属于哪个版本。
- **Cache/Bloom** 用来降低读放大和磁盘 I/O。

---

## 2. 核心概念

| 概念 | 作用 |
|---|---|
| MemTable | 内存中的有序表，通常用 SkipList；承接最新写入 |
| Immutable MemTable | 已冻结、等待 flush 的 MemTable |
| WAL / LOG | 预写日志；写 MemTable 前先落盘，保证崩溃恢复 |
| SSTable | 有序、不可变的磁盘表；LSM 的持久化单元 |
| L0 | flush 输出层；文件之间 key range 可能重叠 |
| L1-L6 | 磁盘层；层内 key range 不重叠，层间按 key 查找 |
| Manifest | 记录当前 Version 的文件列表、日志号、下一个文件号、最后序列号 |
| CURRENT | 一个很小的文件，内容是当前 Manifest 文件名 |
| Version | 某一时刻的 SSTable 文件集合 + 层级关系 |
| VersionEdit | Manifest 中的增量变更：增加/删除文件、更新 log number 等 |
| Sequence | 写入序号；用于快照、MVCC、compaction 判断新旧 |
| Snapshot | 一个逻辑读视图，对应某个 sequence |
| InternalKey | `(user_key, sequence, type)` 的编码；SSTable 中按它排序 |
| Compaction | 把多个 SSTable 合并成新 SSTable，去重、丢旧版本/tombstone |
| Block Cache | 缓存 SSTable data block |
| Table Cache | 缓存打开的 SSTable 文件和索引 |
| BloomFilter | 快速判断 key 是否可能存在于某个 SSTable/filter |
| WriteBatch | 多 key 原子写；一次 WAL 记录、一次 MemTable 批量插入 |

### 2.1 InternalKey 与 Sequence

LevelDB 的 key 不是简单的 user key，而是：

```text
InternalKey = user_key + (sequence << 8 | type)
```

- `type`：`kTypeDeletion=0`、`kTypeValue=1`（后续版本还有别的类型）。
- `sequence`：写入序号，越大越新。
- 同一 user key 的多个版本按 **sequence 降序** 排列（新的在前）。
- 读取时用 `(user_key, snapshot_seq)` 查第一个 `sequence <= snapshot_seq` 的版本。
- compaction 时保留最高 sequence 的版本；如果最高版本是 tombstone 且没有更老快照需要，可以丢弃。

这是 LevelDB 能做 Snapshot、正确 compaction、正确删除的关键。

---

## 3. 写路径

### 3.1 一次 Put 的主流程

```text
DB::Put(options, key, value)
  → 构造 WriteBatch（单条 Put 也是一个 batch）
  → DBImpl::Write
      → 获取 writer 锁（同一时间只有一个 writer）
      → BuildBatchGroup：合并其它等待中的写（group commit）
      → MakeRoomForWrite：必要时等待/冻结 MemTable
      → 将 batch 追加到 WAL
      → 如果 write_options.sync：WAL fsync
      → 将 batch 插入 MemTable
      → 更新 last_sequence
      → 释放 writer 锁，唤醒等待者
```

### 3.2 WAL 先于 MemTable

顺序必须是：

```text
WAL append → MemTable insert
```

原因：

- 如果先写 MemTable 再写 WAL，进程在两步之间崩溃，内存数据丢失、日志也没有。
- WAL 写成功但 MemTable 未插入：重启重放 WAL 可以恢复。
- WAL 写失败：直接返回错误，不修改 MemTable。

### 3.3 Group Commit

LevelDB 的 writer 是单线程串行的，但支持把多个写请求合并成一个 batch group：

- 减少 WAL 系统调用次数；
- `sync=true` 时，一次 fsync 可以服务多个写请求；
- 提高小写入吞吐。

### 3.4 MakeRoomForWrite 与写停顿

当 MemTable 达到 `write_buffer_size`：

1. 如果 L0 文件过多，先放慢/停止写入（write slowdown / write stall）；
2. 把当前 MemTable 变成 Immutable MemTable；
3. 创建新的 MemTable 和新的 WAL；
4. 调度后台线程把 Immutable MemTable flush 成 L0 SSTable；
5. 如果 L0 文件数达到阈值，触发 compaction。

写停顿是 LSM 的重要工程问题：写入速度长期高于 compaction 速度时，L0 会堆积，读放大和空间放大会恶化。

### 3.5 WriteBatch

一个 WriteBatch 包含：

```text
| sequence (8B) | count (4B) | record 1 | record 2 | ... |
record = | type (1B) | key_len (varint) | key | value_len (varint) | value |
```

- 一个 batch 共享起始 sequence；
- 一次 WAL 记录、一次 MemTable 批量插入；
- 支持原子多 key 写（要么全部可见，要么全部不可见）。

---

---

## 4. 读路径

### 4.1 Get 的查找顺序

```text
DB::Get(options, key, value)
  → 取当前 Snapshot / sequence
  → 查 MemTable
  → 查 Immutable MemTable（新的在前）
  → Version::Get
      → L0 文件：从新到旧逐个检查
      → L1-L6：每层二分查找 key range，再读对应文件
  → 命中 Put：返回 value
  → 命中 Delete：返回 NotFound
  → 都没有：NotFound
```

### 4.2 每层怎么查

- **L0**：文件之间可能重叠，必须从新到旧逐个检查。
- **L1-L6**：层内文件按 key range 不重叠，可以用每个文件的 `largest_key` 二分定位一个文件。
- 每个 SSTable 查询：
  1. 查 BloomFilter（快速否定）；
  2. 查 index block（定位 data block）；
  3. 查 block cache；
  4. 读 data block，按 restart point / 前缀压缩解析；
  5. 找到 InternalKey 后按 sequence 判断版本。

### 4.3 读放大

一次 miss 可能访问：

```text
MemTable + Immutable + 多个 L0 文件 + L1..L6 各一个文件
```

读放大（read amplification）是 LSM 的核心成本之一。LevelDB 通过：

- 限制 L0 文件数；
- leveled compaction 保证层内不重叠；
- BloomFilter；
- Block Cache；
- Table Cache

来降低读放大。

### 4.4 Iterator 与 MergeIterator

`DB::NewIterator` 不是简单遍历一个文件，而是：

- MemTable iterator；
- Immutable MemTable iterator；
- 每个 SSTable 的 `TableIterator`；
- 多个 iterator 通过 `MergeIterator` 做 k 路归并；
- 相同 user key 按 sequence 选版本；
- 对 Snapshot 过滤 `sequence > snapshot` 的版本；
- 跳过 tombstone；
- 输出用户可见的 `(key, value)`。

---

## 5. MemTable 与 SkipList

### 5.1 为什么 LevelDB 用 SkipList

LevelDB 的 MemTable 是单写者、多读者：

- 同一时间只有一个 writer 插入；
- 多个 reader 可以无锁并发读；
- 不需要删除（MemTable 最终整体丢弃）；
- 插入只需要修改有限的 `next` 指针；
- 不需要 AVL/红黑树的旋转和再平衡；
- 范围遍历天然有序。

因此 LevelDB 实现了一个**极简、无锁读、单写者**的 SkipList，而不是通用的平衡树。

### 5.2 LevelDB SkipList 的关键设计

- 节点高度随机，最大高度默认 12；
- 使用 arena 分配节点，减少 malloc 开销和内存碎片；
- 插入时只写新节点和有限的 `next` 指针；
- 读路径使用原子指针/内存屏障，避免读锁；
- key 是 InternalKey，按 `(user_key asc, sequence desc, type desc)` 排序；
- 不支持删除；MemTable 刷盘后整体释放。

### 5.3 与“通用 SkipList”的区别

通用 SkipList 往往要考虑：

- 多写者并发插入；
- 删除；
- 内存回收；
- 迭代器生命周期；
- 泛型 key/value/allocator。

LevelDB 的 MemTable SkipList 把这些都去掉了，只保留 MemTable 需要的最小能力，所以实现短、路径快、容易验证。

### 5.4 wedb 的 MemTable

wedb v0.1.0 使用 `data_structure` 包里的 `ds::SkipList`（0.1.2）：

- 修复了越界、随机高度、erase 悬空、clear 等问题；
- MemTable 自定义 `BytewiseComparator`；
- 单写锁，不追求无锁读；
- 后续可以给 SkipList 增加 `lower_bound` 优化 Scan 起点。

---

## 6. WAL（Write-Ahead Log）

### 6.1 日志块格式

LevelDB 的 WAL 按 32KB block 组织。每个物理记录头 7 字节：

```text
| checksum (4B) | length (2B) | type (1B) | payload |
```

- `checksum`：CRC32C（LevelDB 使用 masked CRC32C）；
- `length`：payload 长度；
- `type`：
  - `FULL=1`：完整记录；
  - `FIRST=2`：分片的第一段；
  - `MIDDLE=3`：中间段；
  - `LAST=4`：最后一段；
- 一个逻辑记录跨 block 时会被拆分；
- block 尾部不足 7 字节时补 0；
- 读取时遇到不完整/CRC 错误的尾部记录，视为 torn tail，停止重放。

### 6.2 写入与恢复

- 写 WAL 后，可选 `fsync`（`WriteOptions.sync`）；
- MemTable flush 成功并更新 Manifest 后，旧 WAL 才能删除；
- 恢复时从 `log_number` 开始重放 WAL；
- 重放顺序必须与写入顺序一致；
- 重放完成后恢复 `last_sequence`。

### 6.3 WAL 的取舍

| 选项 | 优点 | 缺点 |
|---|---|---|
| `sync=false` | 吞吐高 | 掉电可能丢最后一段 |
| `sync=true` | 持久化强 | 每次写等待磁盘 |
| group commit | 摊薄 fsync 成本 | 需要写合并逻辑 |
| WAL 分片 | 支持大 batch | 读取/恢复逻辑更复杂 |

wedb v0.1.0 的 WAL 更简单：一条 Entry 一个记录，不做 block 分片；支持 CRC、重放和 torn tail，满足自用场景。

---

## 7. SSTable

SSTable（Sorted String Table）是 LevelDB 的磁盘数据单元：**有序、不可变、按 block 组织**。

### 7.1 文件整体布局

```text
+-------------------+
|   data block 0    |
+-------------------+
|   data block 1    |
+-------------------+
|        ...        |
+-------------------+
|   filter block    |  BloomFilter 数据
+-------------------+
|   meta index block|  指向 filter block 的索引
+-------------------+
|    index block    |  指向 data block 的索引
+-------------------+
|      footer       |  固定 48 字节，指向 meta index + index
+-------------------+
```

### 7.2 Data Block

一个 data block 由多条有序 key/value 记录组成：

```text
| entry 0 | entry 1 | ... | entry n |
| restart point 数组 |
| restart count (4B) |
| compression type (1B) |
| crc32c (4B) |
```

LevelDB 的 entry 使用前缀压缩：

```text
shared_bytes | unshared_bytes | value_length | key_delta | value
```

- `shared_bytes`：与上一个 key 共享的前缀长度；
- `unshared_bytes`：本 key 剩余长度；
- restart point：每 N 条记录存一个完整 key，便于二分/顺序扫描；
- 默认 restart interval 为 16。

**为什么需要 restart point**：前缀压缩后，要定位中间 key 不能直接按字节偏移二分；restart 数组提供“完整 key 的锚点”，查 index 后从最近的 restart 开始扫描。

### 7.3 Filter Block

- 存储 BloomFilter 数据；
- LevelDB 的 `FilterPolicy` 可以按 block 生成 filter；
- 通过 `filter_offset` 定位；
- 查询时先查 filter，再查 index/data block；
- filter 只做“快速否定”，不允许假阴性。

### 7.4 Meta Index Block 与 Index Block

- **meta index**：记录 filter block 的 offset/size；
- **index block**：每个 data block 一条记录：
  - data block 的最大 key；
  - data block 在文件中的 offset；
  - data block 的大小。

查询时：

1. 在 index block 中二分，找到第一个 `last_key >= target` 的 block；
2. 读该 data block；
3. 在 block 内按 restart/前缀压缩解析。

### 7.5 Footer

LevelDB footer 固定 48 字节，包含：

- meta index block handle（offset + size）；
- index block handle；
- padding；
- 8 字节 magic。

文件末尾固定长度，方便从文件尾反向定位索引。

### 7.6 压缩与校验

- 每个 block 有 1 字节 compression type；
- 可选 Snappy 压缩；
- 每个 block 末尾有 CRC32C；
- 读取时校验，损坏返回 Corruption；
- SSTable 整体不可变，修改只能通过 compaction 生成新文件。

### 7.7 写入流程

```text
TableBuilder
  → 逐条 Add
  → 达到 block_size（默认 4KB）就 flush 一个 data block
  → 写 filter block
  → 写 index block
  → 写 footer
  → fsync
  → rename 成正式 .sst
```

---

## 8. BloomFilter

BloomFilter 用来减少“不存在的 key”造成的磁盘读取。

### 8.1 参数

- `bits_per_key`：每个 key 分配多少 bit，LevelDB 默认 10；
- `k`：哈希函数个数，通常取 `bits_per_key * ln2 ≈ 6`；
- 使用多个 hash 位置；
- 只允许“假阳性”，不允许“假阴性”。

误判率近似：

```text
false_positive ≈ (1 - e^(-k*n/m))^k
```

其中 `n` 是 key 数，`m` 是 bit 数。

### 8.2 LevelDB 的实现特点

- filter 按 data block 分组，而不是整个文件一个 filter；
- filter block 中保存每组的 offset；
- 查询时根据 data block 在文件中的 offset 找到对应 filter；
- 可以在 `Options` 中替换 `FilterPolicy`。

### 8.3 常见误区

- BloomFilter 不能回答“key 一定存在”，只能说“可能存在”；
- BloomFilter 不能替代 index；
- BloomFilter 对范围扫描帮助有限；
- 过滤器失效/损坏时必须降级为正常查找，不能产生假阴性。

wedb v0.1.0 每个 SSTable 一个 BloomFilter，够用但粒度较粗；TODO 中列入“每 block 一个 filter”。

---

## 9. Cache

LevelDB 有两层与缓存相关的结构：

### 9.1 Block Cache

- 缓存 SSTable 的 data block；
- key 通常是 `(file_number, block_offset)`；
- 默认 8MB LRU；
- 读取时先查 cache，未命中再读磁盘并插入；
- 缓存需要引用计数，避免 compaction 删除文件后 block 仍被使用。

### 9.2 Table Cache

- 缓存打开的 SSTable（文件句柄、index、filter）；
- 避免每次 Get 都重新打开文件、读取 footer/index；
- 受 `max_open_files` 限制；
- 文件被 compaction 删除后，需要等引用释放再真正关闭。

### 9.3 缓存与 OS Page Cache

- LevelDB 的 Block Cache 是用户态缓存；
- OS 也有 page cache；
- 两层缓存叠加，热点数据可能命中用户态缓存，冷数据命中 OS cache 或磁盘；
- 基准测试要区分 cache 命中/未命中。

wedb v0.1.0 不做通用 Block Cache，但 **TableReader 打开时缓存 index/filter**；数据 block 依赖 OS page cache。TODO 中列入 LRU Block Cache。

---

## 10. Compaction

Compaction 是 LSM 最复杂、也最影响性能和正确性的部分。

### 10.1 两类 compaction

| 类型 | 触发 | 作用 |
|---|---|---|
| Minor compaction | MemTable 满 | 把 Immutable MemTable flush 成 L0 SSTable |
| Major compaction | L0/某层文件数或容量达到阈值 | 把多个 SSTable 合并，向下一层下沉 |

### 10.2 Leveled Compaction

LevelDB 使用 leveled compaction：

- **L0**：文件之间 key range 可以重叠；查询时从新到旧逐个检查。
- **L1-L6**：每层内部 key range 不重叠；查询时二分定位一个文件。
- **容量比例**：通常每层容量是上一层的 10 倍左右；L1 基础大小 10MB（可配置）。
- **L0 触发**：L0 文件数达到 4（`kL0_CompactionTrigger`）触发。
- **单文件大小**：目标文件 2MB 左右（`max_file_size`），合并输出可切分。
- **限制 L0 数量**：L0 达到 8 个文件减慢写入，12 个停止写入（LevelDB 默认阈值）。

### 10.3 一次 compaction 的步骤

```text
1. 选择 level 和输入文件
   - L0：选最新的若干文件
   - L1-L6：选一个文件，并扩展到目标层中 key range 重叠的文件
2. 构造 MergeIterator
   - 多路归并，按 InternalKey 排序
   - 同一 user key 按 sequence 选新版本
3. 处理 tombstone
   - 如果还有更老的快照需要这个 key，保留 tombstone
   - 如果到最底层且没有快照需要，丢弃 tombstone 和旧版本
4. 写输出 SSTable
   - 按目标文件大小切分
   - fsync + rename
5. 提交 VersionEdit
   - 增加输出文件
   - 删除输入文件
   - 更新 log number / next file number / last sequence
6. 删除 physical 输入文件
   - 必须等 Manifest 提交成功、无读者引用后再删
```

### 10.4 Compaction 与 Snapshot

- 删除/覆盖 key 时，旧版本不能立即丢，因为可能有 Snapshot 需要读旧版本。
- Version/DBImpl 维护“最老快照序列号”；
- compaction 只丢弃所有活跃快照都看不到的旧版本；
- tombstone 只有在最底层且没有快照需要时才丢弃。

### 10.5 三种放大

| 放大 | 含义 | 影响因素 |
|---|---|---|
| 写放大 | 一份用户数据被写多少次 | compaction 频率、层级、整层/leveled |
| 读放大 | 一次读要访问多少文件/块 | 层数、L0 文件数、Bloom/Cache |
| 空间放大 | 磁盘上保留多少旧版本/tombstone | compaction 是否及时、快照保留 |

LSM 的本质就是在三者之间做权衡。LevelDB 选择 leveled compaction，偏向较低读放大和空间放大，但写放大较高。

### 10.6 wedb v0.1.0 的简化

- 采用**整层合并**：某层文件数达到阈值，合并该层全部文件成一个输出；
- 不做 L0/L1 重叠文件选择；
- 同步执行，无后台 compaction；
- 写放大会比 LevelDB 高，但规则少、正确性容易验证。

---

## 11. Version / Manifest

### 11.1 为什么需要 Version

SSTable 文件会不断增加/删除。DB 必须知道：

- 当前有哪些文件；
- 每个文件在哪一层；
- 每个文件的 key range、大小、最大 sequence；
- 当前 WAL 编号、下一个文件编号、最后 sequence。

`Version` 就是“某一时刻的文件集合 + 层级状态”。

### 11.2 Manifest 与 CURRENT

- `MANIFEST-XXXXXX`：记录 VersionEdit 的追加日志；
- `CURRENT`：内容是一个 Manifest 文件名；
- 启动时读 CURRENT → Manifest → 重建 Version；
- VersionEdit 包括：
  - AddFile / DeleteFile；
  - SetLogNumber；
  - SetNextFile；
  - SetLastSequence；
- Manifest 提交是 compaction/flush 的原子点。

### 11.3 恢复流程

```text
1. 获取 LOCK
2. 读 CURRENT，找到 Manifest
3. 重放 Manifest 中的 VersionEdit，重建 Version
4. 打开当前 Version 的所有 SSTable
5. 重放 log_number 之后的 WAL
6. 删除 manifest 中不存在的孤儿文件
7. 必要时写新的 Manifest / 触发 flush
```

### 11.4 引用计数

- Version 可能被读请求/迭代器持有；
- 文件可能被 TableCache/BlockCache 持有；
- compaction 删除文件前必须确认没有引用；
- LevelDB 通过 Version 引用计数、文件引用计数、cache 引用计数管理生命周期。

### 11.5 wedb v0.1.0 的简化

- 单个行式文本 `MANIFEST`，全量重写 + 原子 rename；
- 无 CURRENT；
- 无 append-only VersionEdit；
- 无 Version 引用计数（单线程、无后台 compaction）；
- 文件很小，flush/compaction 时重写成本可接受。

---

## 12. Iterator 与 Snapshot

### 12.1 Iterator 层次

```text
用户 Iterator
  → Snapshot / sequence 过滤
  → MergeIterator（多路归并）
      → MemTable Iterator
      → Immutable MemTable Iterator
      → TableIterator（每个 SSTable）
          → IndexBlock 二分
          → DataBlock 解析
```

### 12.2 Snapshot

- Snapshot 本质是一个 sequence 号；
- `DB::Get(options, key)` 使用当前 sequence；
- `DB::NewIterator(options)` 可以绑定某个 Snapshot；
- compaction 必须保留活跃 Snapshot 能看到的旧版本；
- Snapshot 不复制数据，只记录版本视图。

### 12.3 Tombstone 与删除

- Delete 写入一条 `kTypeDeletion` 记录，而不是立即物理删除；
- 读路径遇到最高 sequence 的 tombstone 返回 NotFound；
- compaction 合并时：
  - 如果还有更老版本/Snapshot，保留 tombstone；
  - 如果到最底层且无 Snapshot，丢弃 tombstone 和更旧版本。

---

## 13. 并发与线程模型

LevelDB 的并发设计：

- **单 writer**：同一时间只有一个写线程进入写路径；
- **多 reader**：读路径可以并发；
- **MemTable SkipList**：单写者 + 原子指针，读者无锁；
- **后台 compaction**：独立后台线程执行 flush/compaction；
- **mutex**：保护 Version、文件列表、MemTable 切换等元数据；
- **write stall**：L0 文件过多时主动放慢/停止写入；
- **引用计数**：Version/TableCache/BlockCache 的生命周期管理。

wedb v0.1.0：

- 单进程、单写锁；
- 所有 Put/Get/Delete/Scan 串行；
- flush/compaction 同步执行；
- 没有后台线程、没有引用计数、没有 write stall；
- 用简单换正确性，性能优化见 TODO。

---

## 14. 编码、API 与错误处理

### 14.1 编码

LevelDB 内部大量使用：

- **Fixed32 / Fixed64**：固定宽度小端整数；
- **Varint32 / Varint64**：变长整数，常用于 block 内 key/value 长度；
- **InternalKey**：user key + sequence + type；
- **CRC32C**：块/记录校验；
- **BlockHandle**：`offset + size`，指向文件内一个 block。

显式编码而不是直接 `memcpy` 结构体，保证跨平台和格式稳定。

### 14.2 API

LevelDB 的核心 API：

```cpp
DB::Open(options, path, &db);
db->Put(write_options, key, value);
db->Get(read_options, key, &value);
db->Delete(write_options, key);
db->Write(write_options, &batch);
db->NewIterator(read_options);
db->GetSnapshot();
db->ReleaseSnapshot(snapshot);
db->CompactRange(...);
db->Close();
```

配套类型：

- `Options`：write buffer、block size、cache、filter policy、compression、max_open_files；
- `ReadOptions`：verify_checksums、fill_cache、snapshot；
- `WriteOptions`：sync；
- `WriteBatch`：批量写；
- `Status`：错误码 + 消息；
- `Iterator`：Seek/Next/Valid/key/value/status。

### 14.3 错误处理

- 所有磁盘/格式错误都通过 `Status` 返回；
- 读路径校验 CRC；
- 写路径区分 WAL 失败、MemTable 失败、后台 compaction 失败；
- 后台错误会记录在 DB 状态中，后续操作返回该错误；
- 恢复失败通常拒绝打开，避免误删数据。

wedb v0.1.0 用 `bool + ErrorCode`（`basic::ErrHandle`）而不是完整 `Status`，接口更简单；未来可引入 `Status`。

---

## 15. 崩溃恢复

LevelDB 的恢复依赖三样东西：

1. **WAL**：恢复已写但未 flush 的数据；
2. **Manifest/CURRENT**：恢复文件列表和层级；
3. **原子 rename/fsync**：保证不会把半成品当正式文件。

关键故障窗口：

| 崩溃点 | 状态 | 恢复行为 |
|---|---|---|
| WAL 写入中 | 尾部可能不完整 | 停止重放 torn tail |
| MemTable flush 后、Manifest 提交前 | 新 SSTable 是孤儿 | 删除/忽略孤儿 |
| Manifest 提交后、删除旧 WAL 前 | 旧 WAL 残留 | 按 log_number 跳过/清理 |
| compaction 输出后、Manifest 提交前 | 输出 SSTable 孤儿 | 删除/忽略 |
| Manifest 提交后、删除输入文件前 | 输入文件残留 | 按 Manifest 清理 |

核心原则：

- **Manifest 提交是文件列表变更的原子点**；
- 写入顺序必须是“临时文件 → fsync → rename → fsync 目录”；
- 删除文件必须在 Manifest 提交之后；
- 启动时清理孤儿/临时文件。

wedb v0.1.0 已实现 WAL 重放、MANIFEST 原子替换、SSTable 临时文件 + rename；未实现完整的孤儿文件清理/repair。

---

## 16. 性能模型

### 16.1 关键参数（LevelDB 默认量级）

| 参数 | 默认/量级 |
|---|---|
| write_buffer_size | 4MB |
| block_size | 4KB |
| block_cache | 8MB |
| bits_per_key | 10 |
| L0 compaction trigger | 4 |
| L0 slowdown / stop | 8 / 12 |
| max_file_size | 2MB |
| level size multiplier | 10 |
| max_open_files | 1000 |

### 16.2 性能特征

- **写路径**：WAL 顺序写 + MemTable 内存插入，通常很快；`sync=true` 受 fsync 限制。
- **读路径**：MemTable 命中很快；SSTable 读受 Bloom、block cache、OS page cache 影响。
- **compaction**：后台执行，但会消耗 CPU/磁盘带宽；compaction 跟不上时触发写停顿。
- **写放大**：leveled compaction 可能把数据重写多次；整层合并也类似。
- **读放大**：层数越多、L0 文件越多，读放大越大；Bloom/cache 可以缓解。
- **空间放大**：旧版本/tombstone 未及时清理会占空间。

### 16.3 优化手段

- 增大 write buffer，减少 flush 频率；
- 增大 block cache；
- 调 Bloom bits_per_key；
- 压缩（Snappy/Zstd）；
- 限制 L0 文件数 / 写限流；
- leveled compaction 选择更合适的输入；
- 批量写 + group commit；
- 使用 SSD、避免 HDD 随机读。

---

## 17. LevelDB 的局限与 RocksDB 的增强

LevelDB 的定位是**单进程、嵌入式、轻量 KV**，不是完整数据库。

### 17.1 LevelDB 局限

- 单进程（LOCK 独占）；
- 无跨 batch 事务；WriteBatch 只保证单个 batch 内原子；
- 无列族；
- 无 TTL；
- 压缩以 Snappy 为主，可扩展但内置算法有限；
- 单后台 compaction 线程（实现相对简单）；
- 无完善统计/监控；
- 无多线程 compaction；
- 无 Blob/大 value 分离存储；
- 无内置 repair 工具（有 `RepairDB` 但能力有限）。

### 17.2 RocksDB 的增强

RocksDB 从 LevelDB 演进而来，增加了：

- Column Families；
- 事务（TransactionDB/OptimisticTransactionDB）；
- Merge Operator；
- 多线程 compaction / 更丰富的 compaction 策略；
- Rate Limiter / Write Stall 调优；
- Block-based table 的 partitioned index/filter；
- 压缩算法和压缩策略更丰富；
- BlobDB（大 value 分离）；
- Backup/Checkpoint；
- Statistics/Perf Context；
- 更丰富的 MemTable（HashSkipList、HashLinkList、VectorRep）；
- 更完善的工具和监控。

如果目标是生产级，通常直接用 RocksDB/LevelDB，而不是自己实现。

---

## 18. LevelDB 技术点与 wedb v0.1.0 对照

| 技术点 | LevelDB | wedb v0.1.0 | 后续 |
|---|---|---|---|
| WAL | block 分片、CRC32C、group commit | 单记录 WAL、CRC32、重放 | group commit / 分片 |
| MemTable | 单写者无锁读 SkipList、arena | `ds::SkipList`（0.1.2） | lower_bound / arena |
| InternalKey / Sequence | 支持 Snapshot/MVCC | 有 seq，无 Snapshot | Snapshot/MVCC |
| SSTable data block | 前缀压缩 + restart point | 无前缀压缩/restart | 前缀压缩 + restart |
| Filter | 按 block/filter policy | 每 SSTable 一个 | 每 block filter |
| Index/Meta/Footer | 完整 | 完整（简化版） | 多级 index |
| Compression | Snappy | 无 | Zstd/Snappy |
| Block Cache | LRU sharded | 无通用 cache | LRU Block Cache |
| Table Cache | 有 | readers_ 常驻 | max_open_files |
| Manifest | append-only VersionEdit + CURRENT | 行式文本全量 MANIFEST | append-only / 备份 |
| Compaction | leveled，L0-L6 | 整层合并，L0-L5 | leveled compaction |
| Snapshot | 支持 | 不支持 | P2 |
| WriteBatch | 支持 | 不支持 | P2 |
| 并发 | 单 writer、多 reader、后台线程 | 单写锁串行 | 后台 flush/compaction |
| 崩溃恢复 | WAL + Manifest + 孤儿清理 | WAL + MANIFEST 原子替换 | 故障注入/repair |
| 统计/监控 | 有限 | 无 | metrics |
| 生产工具 | RepairDB 等 | 无 | backup/repair |

wedb 的目标不是复刻 LevelDB，而是：

- 用可读的代码实现 LSM 的核心链路；
- 在自用场景下保证正确、可恢复；
- 把 LevelDB 的复杂技术点作为 TODO 逐步演进。

---

## 19. 推荐阅读路线

按 `doc/leveldb-handbook` 的章节：

1. **基本概念**：MemTable、Immutable、WAL、SSTable、Manifest、CURRENT。
2. **读写操作**：写路径、读路径、WriteBatch、Snapshot。
3. **日志**：日志格式、写日志、读日志、恢复。
4. **内存数据库**：SkipList、MemTable、InternalKey。
5. **SSTable**：data block、filter block、meta index、index block、footer。
6. **Cache**：Block Cache、LRU、引用计数。
7. **BloomFilter**：结构、参数、误判率。
8. **Compaction**：minor/major、leveled、tombstone、快照。
9. **版本控制**：Version、Manifest、CURRENT、恢复、引用计数。

对应 LevelDB 源码入口（供后续深入）：

```text
db/db_impl.cc         写路径、读路径、compaction 调度
db/memtable.cc        MemTable
db/skiplist.h         MemTable SkipList
db/log_writer.cc      WAL 写
db/log_reader.cc      WAL 读
table/table_builder.cc SSTable 写
table/table.cc         SSTable 读
db/table_cache.cc      Table Cache
db/version_set.cc      Version / Manifest / compaction picker
db/db_iter.cc         用户 Iterator / MergeIterator
util/cache.cc         LRU Cache
util/bloom.cc         BloomFilter
```

---

## 20. 结论

LevelDB 的技术栈可以概括为：

```text
WAL + MemTable(SkipList) + SSTable
+ Manifest/Version
+ Compaction
+ Bloom/Cache
+ Snapshot/Sequence
+ Iterator
+ 崩溃恢复
+ 并发/写停顿/引用计数
```

- **最简单的可用内核**：WAL + MemTable + SSTable + Manifest + compaction；
- **决定性能和正确性的难点**：InternalKey/sequence、compaction、Version/Manifest、崩溃恢复；
- **决定生产能力的工程点**：cache、bloom、group commit、write stall、repair、metrics。

wedb v0.1.0 完成了第一层“可用内核”的自用简化版；后续按 `TODO.md` 逐步补齐性能、快照、缓存、leveled compaction 和工程化能力。

