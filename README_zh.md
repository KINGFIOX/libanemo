# `libanemo` - 通用CPU模拟器与差分测试框架

> 但风向是会转变的。
>
> 终有一天，会吹向更有光亮的方向。
>
> 从今往后，带着我的祝福，活得更加从容一些吧。

## 概述

`libanemo`是一个旨在为处理器设计提供统一、可移植且优雅工具集的库。其设计注重可扩展API与差分测试支持，包含以下组件：

- `libvio`：具备统一接口与差分测试支持的模拟外设
- `libcpu`：采用统一接口的模拟CPU核心
- `libsdb`：类GDB的简易命令行调试接口

## 快速开始

### 构建指南

使用此库最快捷的方式是将其作为CMake子目录包含到您的项目中，并链接生成的静态库。若您的项目使用的构建系统不是CMake，只需配置您的构建系统以编译所有源文件并将`include`目录添加到头文件路径中。目前本项目无需任何外部依赖库。若系统中缺少`elf.h`文件，可从Linux系统复制该文件至`include`目录。

### 使用虚拟MMIO

`libvio` 包含四个主要部分：

- IO前端：处理MMIO请求。
- IO后端：执行实际的输入和输出操作。
- MMIO派发器：将MMIO请求分发给一组受其管理的前端。
- MMIO代理：为模拟处理器提供接口。

可以为同一类型的虚拟设备实现不同的IO后端。
例如，本库中实现了一个基于iostream的简单控制台IO后端。
你也可以实现另一个控制台IO后端，将模拟处理器的虚拟控制台连接到计算机的物理串口。
两种后端均可与同一前端配合使用。

要使用`libvio`进行模拟MMIO，只需初始化一个`libvio::io_dispatcher`，
并为每个虚拟设备指定IO前端、IO后端、起始地址和地址范围大小。

```c++
#include <libvio/bus.hh>

libvio::io_dispatcher dispatcher{{
    {new libvio::console_frontend{}, new libvio::console_backend_iostream{std::cin, std::cout}, 0xa00003f8, 8},
    {new libvio::mtime_frontend{}, new libvio::mtime_backend_chrono{}, 0xa0000048, 16}
}};
```

然后通过`io_dispatcher.new_agent()`创建该派发器的MMIO代理。

```c++
auto agent = dispatcher.new_agent();
```

之后即可使用`agent.read()`和`agent.write()`来模拟MMIO操作。

### 模拟CPU

`libcpu`提供了用于模拟处理器核心的`abstract_cpu`基类，以及用于模拟内存的`abstract_memory`基类。
要模拟一个处理器，需要实例化一个处理器核心和一个虚拟内存，用适当的内容初始化内存，并将内存连接到CPU核心的内存端口。

```c++
#include <libcpu/rv32i_cpu_system.hh>

libcpu::rv32i_cpu_system cpu;

libcpu::contiguous_memory<uint32_t> memory{0x80000000, 128*1024*1024};
memory.load_elf_from_file(argv[1]);

cpu.instr_bus = &memory;
cpu.data_bus = &memory;
```

可以将一个MMIO代理连接到处理器。如果没有连接MMIO代理，MMIO请求将被忽略。

```c++
cpu.mmio_bus = dispatcher.new_agent();
```

你可以将`instr_bus`和`data_bus`连接到同一块内存，或者连接到各自的缓存（但两个缓存最终连接到同一个内存），
甚至连接到不同的内存（如果处理器使用不同的地址空间来访问指令和数据）。

然后使用`reset()`函数以指定的初始程序计数器重置CPU。
之后，你可以使用`next_instruction()`或`next_cycle()`逐步推进CPU，并通过`stopped()`检查其是否已停止。

```c++
cpu.reset(0x80000000);
while (!cpu.stopped()) {
    cpu.next_cycle();
}
```

`libcpu`在`libcpu/event.hh`中提供了`event_t<WORD_T>`，用于描述架构事件，例如写入寄存器或内存操作。要启用事件追踪，需将一个`libvio::ringbuffer<event_t<WORT_T>>`附加到CPU核心。如果支持，事件将自动放入环形缓冲区。

```c++
libvio::ringbuffer<libcpu::event_t<uint32_t>> events{4096};
cpu.event_buffer = &events;
```

### 访问简易调试命令行

`libsdb` 提供了一个模板类 `sdb<WORD_T>`，用于实现简易的调试命令行界面。

```c++
libsdb::sdb<uint32_t> sdb {};
sdb.cpu = &cpu;

while (!sdb.stopped()) {
    std::cout << "sdb> ";
    std::string cmd;
    std::getline(std::cin, cmd);
    sdb.execute_command(cmd);
}
```

命令格式支持：

- 空格分隔的标记（引号内的部分保留空格）
- 转义序列（`\` 表示后一个字符按原样保留）
- 双引号字符串（`"..."` 用于含空格的单个参数）
- 管道输出（`sdb_command | shell_command`）

例如：

```
break rm 0
eval "sp + a0"
help | less
trace instr | "tee npc.log"
```

使用 `help` 命令或参考 `include/libsdb/sdb.hh` 获取可用命令列表。

## 基于行为的差分测试

该库采用了一种不同的差分测试方法，不是比较CPU的状态，而是比较每条指令的行为。换句话说，它不比较寄存器、CSR等的值，而是比较指令如何修改寄存器的值、执行了哪些内存操作等。选择比较行为的原因如下：

- 对于流水线和超标量处理器，可能不存在具体的“架构状态”。
- 指令可以乱序执行，但必须按序提交。处理器的中间状态可能一片混乱，但指令提交时的行为严格按序且定义明确。
- 这种测试方式下，参考设计无需与待测设计（DUT）保持周期精确同步。只要参考行为正确，甚至可以用单周期参考测试乱序设计。
- 比较整个处理器状态可能带来性能开销。
- 这种方式支持半离线比较，甚至可以在另一个线程中进行。

`libanemo`的设计初衷就是支持这种差分测试方式。`libvio`的设计理念是：当多个MMIO代理连接到同一MMIO派发器时，派发器会比较它们发出的MMIO请求。如果请求序列相同，则从虚拟设备读取的数据保证一致；若序列不同，则触发错误。`libcpu`为模拟器提供统一接口，简化差分测试流程。

`libcpu`提供了`libcpu::abstract_difftest<WORD_T>`类作为差分测试接口。它继承自`abstract_cpu<WORD_T>`，与CPU模拟器接口完全一致，并兼容`libsdb::sdb<WORD_T>`。唯一区别在于它不模拟CPU，而是控制DUT和参考设计并比较它们的行为。`libcpu::simple_difftest<WORD_T>`实现了简单的差分测试逻辑：比较任意待测设计与单周期参考设计对寄存器的写入操作。该逻辑适用于测试大多数类型的处理器。

```c++
#include <libcpu/difftest.hh>
#include <libsdb/sdb.hh>

rv32i_cpu_npc dut{}; // 继承自`abstract_cpu<uint32_t>`的自定义实现
libcpu::rv32i_cpu_system ref{}; // 单周期参考设计

// 初始化DUT和REF

libcpu::simple_difftest<uint32_t> difftest{};
difftest.dut = &dut;
difftest.ref = &ref;

// 不要在DUT和REF初始化时调用`reset`
// `difftest.reset()`会统一重置它们
difftest.reset(init_pc);

libsdb::sdb<uint32_t> sdb {};
sdb.cpu = &difftest;

while (!sdb.stopped()) {
    std::cout << "sdb> ";
    std::string cmd;
    std::getline(std::cin, cmd);
    sdb.execute_command(cmd);
}
```
## 待办事项列表

- 为`libvio`添加中断支持
- RV64I模拟器
- 将指令流转换为静态单赋值形式以进行性能分析
- C语言接口开发
- 为`libvio`添加更多类型的虚拟设备
- 为`libvio`开发类似`nvboard`的SDL后端
- 支持软件模拟外设与RTL实现外设之间的差分测试

## 学术诚信声明

本库作者正在参与“一生一芯”计划，部分代码重构自该计划的课程作业。之所以可以公开此库，是因为其实现方式与官方课程作业要求有显著不同。本项目旨在提供比作业代码更高的通用性和可扩展性。您可以将此库作为辅助工具用于课程作业，例如：

- 使用CPU模拟器作为差分测试参考
- 将`libvio`作为NEMU的替代MMIO框架
- 在作业中使用本项目部分功能（前提是该功能不属于必须自主实现的部分）

若某项作业要求必须独立完成，则您不得：

- 将本项目代码（除简单常量和定义外）直接复制到作业中
- 通过集成本库来完成作业
- 采用与本项目完全相同的实现方式来完成作业要求
