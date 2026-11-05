# Linux 系统编程实验

四个递进难度的小实验，把「进程」「内存布局」「内存分配」这三件事从**抽象概念**变成**能看见的数字**。

全部实验只依赖 Linux 的 `/proc` 文件系统和 POSIX API，没有任何第三方库。适合作为操作系统课的配套动手材料。

---

## 目录

- [实验列表](#实验列表)
- [实验一：fork 基础](#实验一fork-基础)
- [实验二：进程地址空间](#实验二进程地址空间)
- [实验三：/proc/self/maps 解析器](#实验三procselfmaps-解析器)
- [实验四：内存分配行为观测](#实验四内存分配行为观测)
- [编译与运行](#编译与运行)
- [建议的观察方式](#建议的观察方式)

---

## 实验列表

| 目录 | 主题 | 回答的问题 |
|---|---|---|
| `fork_basic/` | 进程创建与写时复制 | 父子的变量为什么互不影响？ |
| `process_pid/` | 虚拟地址空间布局 | 全局、栈、堆的地址分别在哪？ |
| `malloc_check_parser/` | `/proc/self/maps` 解析 | 一个进程到底有多少个内存段？ |
| `malloc_test/` | 内存分配增长 | 一直 `malloc(1MB)` 会发生什么？ |

---

## 实验一：fork 基础

**`fork_basic/fork_basic.cpp`**

```cpp
int value = 10;
pid_t pid = fork();          // 返回两次！

if (pid == 0) {
    value = 20;              // 子进程改自己的副本
    std::cout << "child value" << value << "\n";
    _exit(0);                // 注意：用 _exit 而不是 exit
} else {
    std::cout << "parent value:" << value << "\n";   // 仍是 10
    waitpid(pid, nullptr, 0);
}
```

### 关键点

**1. `fork()` 返回两次**

| 返回位置 | 返回值含义 |
|---|---|
| 子进程 | `0` |
| 父进程 | 子进程 PID |
| 出错 | `-1`（父进程） |

所以 `if (pid == 0)` 判断的是「我在子进程里」，不是「我创建成功了」。

**2. 为什么父子变量互不影响**

`fork()` 之后父子拥有**各自独立的虚拟地址空间**，同一地址 `&value` 在两边指向不同的物理页——这就是**写时复制（Copy-on-Write）**：读的时候共享同一块物理内存（只读），一旦任何一方写入，内核才复制出新页。

打印地址你会发现父子两侧 `&value` **完全相同**——这是虚拟地址，不是物理地址。这个"看起来一样但不共享"的现象是理解虚拟内存最好的切入点。

**3. 子进程必须用 `_exit()`**

`exit()` 会刷新 stdio 缓冲、执行 `atexit` 注册的函数、析构全局对象。子进程是从父进程完整拷贝来的，如果父进程的缓冲区里还有未刷出的数据，子进程 `exit()` 会导致**输出重复打印一次**。

`_exit()` 直接交给内核，不做任何用户态清理——这是 fork 后子进程的标准退出方式。

**4. 用 `waitpid` 避免僵尸进程**

子进程退出后若父进程不回收，会留在进程表里变成 **zombie（Z 状态）**。`waitpid(pid, nullptr, 0)` 阻塞等待并回收。

验证方法：

```bash
./fork_basic &
ps -o pid,ppid,stat,cmd --ppid <父进程PID>
```

把 `waitpid` 注释掉，能直接看到 `Z` 状态的僵尸进程。

---

## 实验二：进程地址空间

**`process_pid/process_pid.cpp`**

把一个进程里各种类型的变量地址同时打印出来：

```cpp
int global_a;         // 未初始化全局（.bss）
int global_b = 100;   // 已初始化全局（.data）
int global_c = 50;    // 同上（.data）

void func() {}

int main() {
    int stack_var = 10;          // 栈
    int *heap_var = new int(20); // 堆
    // 逐个打印 &global_a / &global_b / &global_c / &stack_var / heap_var
    getchar();                   // 暂停，便于另一个终端查看 /proc
}
```

### 典型输出（x86-64 Linux）

```
global_a address: 0x55a1f2c04010    ← .bss，和 .data 靠得很近
global_b address: 0x55a1f2c03008    ← .data
global_c address: 0x55a1f2c0300c    ← .data（与 global_b 相邻 4 字节）
stack address:    0x7ffd4e2a1b4c    ← 栈，高位地址
heap address:     0x55a1f2c0f2b0    ← 堆，紧跟 .bss 之后
```

### 从输出能读出的结论

- **代码段与数据段在低地址**（`0x55...`），栈在高地址（`0x7ff...`），两者相向增长
- `global_a` 与 `global_b/c` 分属 `.bss` 和 `.data`，但地址**几乎连续**——它们在同一个可执行文件的 segment 里
- `global_c` 比 `global_b` 大 4 字节，因为两者都是 `int`——**地址差正好等于类型大小**，说明编译器是按声明顺序紧密排布的（未开启额外对齐填充）
- **栈地址高于堆地址**，且栈向下增长、堆向上增长

### 配合 `getchar()` 暂停，可以在另一个终端验证

```bash
# 终端 1
./process_pid
PID:12345
...

# 终端 2（程序卡在 getchar 时）
cat /proc/12345/maps
```

你会看到打印出的 `0x55...` / `0x7ff...` 地址精确落在 `maps` 里的对应区间内。这是「程序里的指针」和「内核视角的内存映射」对上的那一刻。

---

## 实验三：/proc/self/maps 解析器

**`malloc_check_parser/`** — 本仓库中唯一一个多文件、带类设计的实验。

### 结构

```
MemoryRegion.h      内存段的值对象（地址范围、权限、名称）
MemoryStatistics.h  统计结果（heap / stack / text 段数量）
MemoryParser.h      解析器接口：save / show / calculate
MemoryParser.cpp    从 /proc/self/maps 读取并解析
main.cpp            演示：先创建栈与堆变量，再抓一次快照
```

### 为什么用 `/proc/self/maps`

它把进程的整张虚拟内存映射表以文本形式暴露出来，每行格式：

```
55a1f2c03000-55a1f2c04000 r--p 00001000 08:01 1234567  /usr/bin/program
└───── 地址范围 ─────┘ └权限┘ └offset┘ └设备┘ └inode┘  └─ 映射名（可空）─┘
```

`self` 是内核提供的符号链接，指向当前进程——不需要先 `getpid()`。

### 权限字符串的含义

5 个字符，如 `r-xp`：

| 位置 | 含义 |
|---|---|
| 1 | `r` 可读 |
| 2 | `w` 可写 |
| 3 | `x` 可执行 |
| 4 | `s` 共享 / `p` 私有（写时复制） |
| 5 | 保留 |

**判断代码段就靠第 3 个字符是不是 `x`**：

```cpp
else if (permission.find('x') != std::string::npos) {
    ++statistics.textcount;      // 可执行 = text 段
}
```

而堆和栈在内核里有明确的特殊标记，直接比对名字即可：

```cpp
if (name == "[heap]")        ++statistics.heapcount;
else if (name == "[stack]")  ++statistics.stackcount;
```

### 解析实现要点

用 `ifstream` 逐行读，`stringstream` 按空白切分。注意**映射名可能包含空格**（如带空格的路径），所以前 5 个字段用 `>>` 提取，剩下整行用 `getline` 拿，再手工去掉前导空格：

```cpp
ss >> address >> permission >> offset >> device >> inode;
std::getline(ss, name);
while (!name.empty() && name[0] == ' ') name.erase(0, 1);
```

这个细节是很多 `maps` 解析器的坑点——用 `>>` 读名字会在第一个空格处截断。

### 运行

```bash
cd malloc_check_parser
make
./malloc_check_parser
```

输出包含三个部分：栈/堆变量的实际地址、逐行打印的全部内存段、以及 heap/stack/text 的段数统计。

**通常你会发现 `[stack]` 只有 1 个，而可执行段有 3~5 个**（`r--p` / `r-xp` / `r--p` / `r--p` / `rw-p`，分别对应 ELF 的只读段、代码段、只读数据、重定位信息、可写数据）。

---

## 实验四：内存分配行为观测

**`malloc_test/malloc_test.cpp`**

```cpp
std::vector<void*> v;
while (true) {
    void* p = malloc(1024 * 1024);   // 每次 1 MB，永不释放
    v.push_back(p);
    std::cout << "pid:" << getpid() << std::endl;
    sleep(1);
}
```

### 观察什么

在另一个终端盯住这个进程的 RSS：

```bash
while true; do grep VmRSS /proc/<PID>/status; sleep 1; done
```

你会看到 RSS **稳步上升**，直到触发 OOM Killer 或触及 `ulimit -v` 被 `malloc` 返回 `nullptr` 终止。

### 进阶观察点（代码里的隐藏细节）

**`malloc` 不一定真的分配物理内存。** 小块分配走 `brk`/`mmap` 后，内核只是标记了一段虚拟地址可用，物理页在**首次写入时才真正分配**。所以这个程序里 `malloc` 之后没写数据，`VmSize` 涨得比 `VmRSS` 快得多——这正好演示了「虚拟内存 != 物理内存」。

想做更严格的对照，可以在 `malloc` 后补一句 `memset(p, 0, 1024*1024)`，观察 RSS 与 VmSize 同步增长的差异。

### 安全提示

⚠️ 这个程序会**持续吃内存直到系统崩溃**。请务必在虚拟机或容器里跑，并设好保护：

```bash
ulimit -v 524288      # 限制虚拟内存 512 MB，进程超限即失败退出
./malloc_test
```

---

## 编译与运行

每个目录都带独立的 `Makefile`：

```bash
cd fork_basic            && make && ./fork_basic
cd process_pid           && make && ./process_pid
cd malloc_check_parser   && make && ./malloc_check_parser
cd malloc_test           && make && ./malloc_test
```

### Makefile 约定

```makefile
cxx=g++
cxxflags=-std=c++17 -O0 -Wall -g
```

`-O0 -g` 是刻意选择的：**关闭优化**才能保证变量不被优化掉、栈布局可观测；**带调试符号**才能配合 `gdb` 查看。

> 注意：`fork_basic/makefile` 用了小写文件名 `makefile`（GNU make 也认，但跨平台不友好），其余目录用 `Makefile`。

### 清理

```bash
cd <目录> && make clean
```

---

## 建议的观察方式

这些实验的**代码本身只有几十行，真正有价值的是「运行后看到了什么」**。推荐搭配下面的工具：

| 工具 | 用途 |
|---|---|
| `cat /proc/<pid>/maps` | 与 `process_pid` 打印的地址互相印证 |
| `grep VmRSS /proc/<pid>/status` | 观察实际物理内存占用 |
| `ps -o stat` | 看僵尸进程的 `Z` 状态 |
| `strace -e trace=brk,mmap,munmap ./malloc_test` | 看 `malloc` 背后到底调了什么系统调用 |
| `valgrind --leak-check=full ./malloc_check_parser` | 本仓库实验都不泄漏，可作对照基线 |
| `pmap -x <pid>` | 更友好的内存映射视图 |

### 一句话推荐路线

```
先跑 fork_basic（理解进程）→ 再跑 process_pid（理解地址空间）
→ 然后 malloc_check_parser（把地址空间量化）→ 最后 malloc_test（看它怎么增长）
```

前两个告诉你「内存长什么样」，第三个告诉你「怎么用代码读它」，第四个告诉你「它怎么变化」。

---

## 已知问题

- **编译产物入库**：四个目录里都有已编译的可执行文件（`fork_basic`、`malloc_test`、`process_pid`、`malloc_check_parser`），不应提交到版本库。建议加 `.gitignore`：

  ```gitignore
  fork_basic/makefile
  malloc_test/malloc_test
  process_pid/process_pid
  malloc_check_parser/malloc_check_parser
  *.o
  ```

- **`MemoryParser` 方法名拼写**：`caulatemessages()` 应为 `calculateMessages()`，`savememory/showmemory` 也建议统一为驼峰。当前命名不影响功能，但读代码时会犹豫。

- **`main.cpp` 未释放 `std::string global_b` 之外的隐式资源**：实验本身就是演示性质，无需修正。

- **仅限 Linux**：`/proc` 依赖 Linux，BSD/macOS 上需要换 `sysctl` 或 `vmmap`。

---

## License

未指定。
