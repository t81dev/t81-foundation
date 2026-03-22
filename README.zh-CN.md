<p align="center">
  <img src="docs/assets/banner.png" alt="T81 — A Ternary Operating System for AI" width="100%">
  QEMU 中的可启动预览 · 受控三元推理 · 跨平台位精确
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation

![发布](https://img.shields.io/badge/release-v1.9.2--Stable-blue)
![测试](https://img.shields.io/badge/tests-404%2F404_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![执行](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![许可证](https://img.shields.io/badge/license-Apache_2.0-blue)

**T81 是人工智能的三元操作系统。**

您加载的每个模型都在受管理的确定性运行时内运行。 Axion 内核会在任何副作用发生之前拦截每个 AI 操作。文件系统是内容寻址且不可变的。 ISA 用加法取代了浮点 matmul，无需乘法单元。任何可以用三元权重表示的人工智能都在这里运行：可验证、可重复且在明确的策略控制下。

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

---

## 演示

在任何 Linux 主机上的 QEMU AArch64 (EDK2 slice6) 上启动 T81：

````嘘
# 安装 deps (Ubuntu 24.04)
sudo apt-get install -y qemu-system-arm qemu-efi-aarch64 mtools cmake ninja-build clang-18 lld-18

# 克隆并运行
git 克隆 https://github.com/t81dev/t81-foundation.git && cd t81-foundation
./drivers/qemu/scripts/boot_demo.sh
````

预期终端输出：

````文本
Axion QEMU AArch64 EDK2 slice6

[axion] 裸机EL1内核入门
[axion] ExitBootServices 完成；移交给 C++ 内核

T81 -- 人工智能三元操作系统
  ===========================

[axion] 策略引擎：准备就绪
[axion] canonfs：已安装（内存中）
[axion] 内核线程 tid=1：正在运行

t81> 帮助
  帮助--这条消息
  版本 -- T81 构建信息
  status——内核计数器和治理状态
  政策 -- Axion 政策摘要
t81>
````

使用 [asciinema](https://asciinema.org) 在本地播放预先录制的会话：

```sh
asciinema play drivers/qemu/t81-boot.cast
```

完整的三相启动日志位于 [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt)。 [`qemu-boot`](.github/workflows/qemu-boot.yml) CI 工作流程会在每次推送时验证此序列。

---

## 目录

- [演示](#演示)
- [AI 缺失的操作系统](#ai-缺失的操作系统)
- [架构](#架构)
- [T81Lang 的样子](#t81lang-的样子)
- [获取 T81](#获取-t81)
- [地位](#状态)
- [启动进度](#启动进度)
- [CLI 参考](#cli-参考)
- [确定性验证](#确定性验证)
- [文档](#文档)
- [治理](#治理)
- [三元优势](#三元优势)
- [执照](#许可证)

## AI 缺失的操作系统

二进制操作系统为人工智能代理提供了一个进程槽和一个文件系统。就是这样。他们无法告诉您推理是否是位精确的、哪个策略授权模型加载，或者磁盘上的权重是否是运行的权重。 T81 缩小了这一差距——不是通过在现有操作系统之上分层工具，而是通过构建 AI 原生计算所需的内核、ISA、文件系统和进程模型。

### 1. 在产生副作用之前控制每个 AI 操作的内核

如今，当人工智能代理采取行动时，通常没有机制可以“事后”验证其计算内容、应用的策略或结果是否被更改。 T81 在指令级别修复了这个问题。

**Axion 内核** *在任何副作用发生之前*拦截 `AgentInvoke`、FFI 调用以及 TISC ISA 中的每个推理操作码。策略是用 Axion 策略语言 (APL) 编写的，并且是失败关闭的 — 策略解析失败会停止操作。每个拦截的事件都会写入 **CanonFS** 锚定的审计跟踪，可以确定性地重播。

```apl
# secure_model.apl — allow inference only for verified model hashes
allow infer if model.hash in approved_models;
deny  infer reason "unapproved-model";
```

```sh
t81 code run inference.t81 --policy secure_model.apl
# Axion: ALLOW  infer  model=sha3:a3f7c2b1…
# Axion: DENY   infer  model=sha3:deadbeef…  reason=unapproved-model
```

### 2. 可重复性作为内核不变量，而不是工具规则

IEEE 754 浮点本质上是平台敏感的：舍入模式不同、非正规处理不同、FMA 可用性会改变结果。无法确定地复制或审核基于其构建的人工智能工作负载。

平衡三元算术围绕零对称。舍入就是截断——没有方向偏差，没有特定于平台的漂移。 T81 的确定性表面生成 **CanonHash81 跟踪哈希，这些哈希在每个受支持的平台上都是位相同的**，并在每次 CI 运行时进行验证。这不是一个可以用螺栓固定的财产；这是 ISA 设计的结果。

```sh
t81 determinism verify-run program.tisc
#  Run 1: a3f7c2b1e94d8f20…
#  Run 2: a3f7c2b1e94d8f20…
#  ✓  bit-exact match confirmed
```

已验证平台：**Linux x86\_64**、**macOS ARM64**。受控确定性表面上的任何分歧都被视为严重缺陷。

### 3. 三元权重原生的 ISA — 无需乘法单元

三元权重 {−1, 0, +1} 没有小数部分。它们的点积是一系列条件加/减运算——不需要乘法。 T81 提供了六个直接利用此功能的 TISC 操作码：

| 操作码 | 手术 |
| :--- | :--- |
| `TWMATMUL` | 三元权重矩阵乘法 |
| `TQUANT` | 将激活量化为 trit |
| `TATTN` | 三元注意力（Q·Kᵀ over trit 权重） |
| `TWEMBED` | 权重嵌入查找 |
| `TERNACCUM` | 标量 trit 点积累积 |
| `TACT` | 使用 Axion 天花板门激活 |

这与 BitNet b1.58 / xTern 类模型一致：与 FP16/FP32 基线相比，**15–60× 能量减少**，**4–90× 吞吐量增益**，且精度相当。 T81 三元权重（T81WTN）格式存储量化模型； `t81 weights import` 从 SafeTensors 或 GGUF 转换。

```sh
t81 weights import model.safetensors -o model.t81w
t81 code run inference.t81 --weights-model model.t81w --policy secure_model.apl
```

---

## 架构

T81 是一个操作系统。每个组件都与传统操作系统设计类似——针对三元语义和人工智能原生工作负载从头开始构建。

| T81 组件 | 操作系统类似物 | 角色 |
| :--- | :--- | :--- |
| **TISC 一** | 指令集（RISC-V、ARM） | 冻结执行合同；所有软件都编译到它 |
| **T81VM** | 内核执行引擎 | 确定性 TISC 解释器； Axion 在每个操作码上触发 |
| **Axion** | 安全内核 | 在出现任何副作用之前采取故障关闭策略；审计锚定 |
| **CanonFS** | 文件系统 | 内容寻址、不可变；通过哈希验证模型权重 |
| **T81Lang** | 系统编程语言 | 编译为 TISC； `agent`/`behavior` 是进程模型 |
| **代理/行为** | 工艺模型 | 代理是一个进程；行为是它的 `main()` |
| **认知层** | 特权环层次结构 | 第 1 层（象征性）→ 第 5 层（分布式）；治理范围 |
| **二聚体** | 调度程序 | 确定性任务图；纪元提交原子性 |

```text
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang  — system language                                 │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
│  agent/behavior (RFC-0015)  ·  foreign {} (RFC-0036)        │
├─────────────────────────────────────────────────────────────┤
│  Axion  — kernel                                            │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81VM  — execution engine   │  DPE  — scheduler            │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA v1.9.0  ❄️ Frozen  +  Data Types  ❄️ Frozen       │
│  CanonHash81 bit-exact traces across all platforms          │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: T81 Userland · Cognitive Tiers
```

**TISC ISA** — 冻结指令集。每个软件都可以编译到它。操作码语义和传输格式在 v1.x 下是不可变的；分歧是一个严重缺陷。

**T81VM** — 执行引擎。 Axion 在操作码调度边界处拦截火灾（在任何副作用之前），将治理路径保持在热解释器循环之外。

**Axion** — 内核。在出现任何副作用之前拦截 `AgentInvoke`、`AXREAD`、`AXSET`、`AXVERIFY`、推理操作码和 FFI 调用。策略解析失败时失败关闭；每项活动均致力于 CanonFS。默认情况下，代理不具备任何功能 - 每个操作都需要明确的策略授权。

**CanonFS** — 文件系统。模型权重、代码对象和运行时工件存储为不可变的、哈希标识的 blob。 Axion 内核验证模型加载的权重是否与管理策略中的哈希值匹配，从而消除操作系统级别的模型交换攻击。

**T81Lang** — 系统编程语言。本机类型：`BigInt`、`Fraction`、`Float`、`Complex`、`Tensor`、`Map`、`Set`、`Option`、`Result`。 `agent`/`behavior` 声明是流程模型——代理是一流流程；行为是其入口点。它们在 TISC 中降至 `AgentInvoke`。 `foreign {}` 块低于 `FFICall` (RFC-00B8)。

**DPE** — 调度程序。任务声明不可变的输入；虚拟机在纪元结束时自动提交所有写入。冻结 ISA 上的确定性并行性 — 不需要新的操作码。

---

## T81Lang 的样子

T81Lang 是T81 的系统编程语言。它编译为 TISC 字节码，并赋予 `agent`/`behavior` 声明一流的地位——代理是一个进程；行为是其入口点。

**基本类型和算术：**

```t81
fn main() -> i32 {
  let greeting: T81String = "Hello, T81!";
  let ratio:    T81Float  = 3.14159t81;
  let big:      T81BigInt = 123456789t81;
  print(greeting);
  print(ratio);
  print(big);
  return 0;
}
```

**代理/行为——流程模型：**

``t81
// 代理是一个命名进程。它的行为是它的入口点。
// Axion 内核策略在执行前对每个 AgentInvoke 进行门控。
代理计算器 {
  行为添加（a：i32，b：i32）-> i32 {
    返回a+b；
  }
}

fn main() -> i32 {
  让结果： i32 = Calculator.add(38, 4);
  打印（结果）；   // 42
  返回0；
}
````

**运行并编译：**

```sh
t81 code run program.t81                          # compile and execute
t81 code build program.t81 -o program.tisc        # compile to bytecode
t81 vm run program.tisc                           # execute bytecode directly
```

**使用 Axion 策略和权重模型：**

```sh
t81 code run inference.t81 \
  --policy        secure_model.apl \
  --weights-model model.t81w \
  --trace
```

**在浏览器中尝试 - 无需安装：**

> **[启动 T81Lang 游乐场 →](https://t81dev.github.io/t81-foundation/playground)**
>
> 直接在浏览器中编写并运行T81Lang程序。完整的编译器 + T81VM 解释器作为 WebAssembly 运行。八个内置示例：Hello World、BigInt 算术、张量、代理/行为等。

**互动探索（本地）：**

```sh
t81 repl       # line-buffered REPL; empty line executes
t81 studio     # human operator TUI (7 views, Ctrl+P palette)
t81 agent      # AI-native TUI with /compile /run /hash /allow /infer
```

---

## 获取 T81

### macOS / Linux

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

检测操作系统和 CPU 架构，下载正确的二进制文件，安装到 `~/.local/bin`。将 `T81_INSTALL_DIR` 设置为覆盖。

### Windows（PowerShell）

```powershell
irm https://github.com/t81dev/t81-foundation/releases/latest/download/install.ps1 | iex
```

安装到 `%LOCALAPPDATA%\t81\bin`。

### Docker — 60 秒，零工具链

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation demo
```

拉取约 100 MB 的映像，运行三个程序（Hello World → 三元类型 → 确定性检查），然后放入交互式 REPL。没有编译器，没有CMake，没有配置。

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation          # REPL only
docker run --rm -it ghcr.io/t81dev/t81-foundation <cmd>    # any t81 subcommand
```

### 预建档案

从[最新版本](https://github.com/t81dev/t81-foundation/releases/latest) 直接下载：

| 平台 | 档案 |
| :--- | :--- |
| Linux x86\_64 | `t81-<version>-linux-x86_64.tar.gz` |
| LinuxARM64 | `t81-<version>-linux-arm64.tar.gz` |
| macOS 苹果芯片 | `t81-<version>-macos-arm64.tar.gz` |
| macOS 英特尔 | `t81-<version>-macos-x86_64.tar.gz` |
| Windows x86\_64 | `t81-<version>-windows-x86_64.zip` |

每个存档都使用标准安装布局：`bin/`、`lib/`、`include/`。将 `bin/t81` 放在您的 `PATH` 上。

### Python（点）

```sh
pip install t81
```

在 Linux（x86\_64、ARM64）、macOS（Apple Silicon、Intel）和 Windows 上安装 CPython 3.9–3.13 的 `t81` Python 包。提供 `T81Int`、`BigInt`、`Float`、`Fraction`、`Tensor`、`HanoiVM`、`CanonFS` 和完整的 `compile`/`compile_and_run` API。每个版本的 Wheel 都会通过 [`python-wheels`](.github/workflows/python-wheels.yml) 工作流程发布到 PyPI。

```python
import t81
result = t81.compile_and_run("fn main() -> i32 { return 42; }")
```

### 从源代码构建

```sh
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # 404 tests
```

---

## 状态

v1.9.2 · 404/404 测试通过 · Apache 2.0

TISC ISA 和核心数据类型在 v1.x 下**冻结** — 操作码语义和线路格式在没有主要版本更新的情况下不会改变。

| 成分 | 到期 | 笔记 |
| :--- | :--- | :--- |
| **TISC 一** | ❄️冷冻 | v1.9.0； `AgentInvoke`，6 个三元本地推理操作码，3 个 FFI，2 个格密码，1 个 NTRU-KEM |
| **数据类型** | ❄️冷冻 | BigInt、Float、Complex、Map、Set — 位稳定编码 |
| **T81VM** | ✅ 稳定 | 经验证的确定性表面； Linux x86\_64 + macOS ARM64 上的位相同跟踪 |
| **T81Lang** | ✅ 稳定 | 规范 v1.9.0；编译器确定性控制活动 |
| **Axion** | ✅ 稳定 | 规范原因字符串、审计挂钩、失败关闭策略执行 |
| **三元本地推理** | ✅ 稳定 | RFC-0034 + RFC-0037；所有 6 个操作码均已实施并得到证实 |
| **格密码学** | ✅ 稳定 | RFC-0038（三元晶格）+ RFC-0039（NTRU-KEM） |
| **受监管的 FFI** | ✅ 稳定 | RFC-00B8 + RFC-0036； `FFIDispatcher`、`FFILibraryRegistry`、`foreign {}` 语法 |
| **DPE（并行执行）** | ✅ 稳定 | RFC-DPE-0001–0009；确定性时代语义 |
| **TUI 前端** | ✅ 测试版 | `t81 studio` 和 `t81 agent` — 生产可用 |
| **认知层** | ✅ 测试版 | 第四层认知 (RFC-0021)；治理范围 |
| **T81 用户区** | ✅ 测试版 | HAL+用户层服务；受政策限制 |
| **本机裸机目标** | 🚧 阿尔法 | T81 目前在 Linux 和 macOS 上作为来宾操作系统层运行；裸机执行正在积极开发中 |
| **QEMU 启动顺序** | 🚧 阿尔法 | EFI → 裸机 → 独立 C++ 桥已确认； `t81>` shell 在串行上运行 — [查看启动进度](#虚拟机检查) |

表面分类遵循 RFC-0048。受控的非 DCP 和实验表面不作为经过验证的确定性组件呈现。

---

## 启动进度

当前 QEMU AArch64 启动序列的实时记录（串行输出）：

<p align="center">
  <img src="https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/assets/boot.gif"
       alt="T81 QEMU AArch64 boot sequence — live t81> shell 演示"
       宽度=“95%”样式=“边框：1px实心#ddd;边框半径：8px；框阴影：0 4px 8px rgba(0,0,0,0.1);">
  <br><small>当前启动进度：EFI→裸机EL1→策略引擎→CanonFS挂载→交互式t81>提示</small>
</p>

<br><small>交互式重播：<a href="https://github.com/t81dev/t81-foundation/blob/main/drivers/qemu/t81-boot.cast">t81-boot.cast (asciinema)</a></small>

T81 在 QEMU AArch64 (EDK2/UEFI) 上启动。下表跟踪干净启动的完成情况，并在串行输出上显示 shell 提示符——这是本自述文件中记录的启动演示的先决条件。

| 阶段 | 它涵盖什么 | 完毕 |
| :--- | :--- | :--- |
| **1. EFI/UEFI 启动** | PE32+ EFI 二进制加载，`ExitBootServices` 完成，切换到裸机内核 | 95% |
| **2.内核入口+HAL初始化** | PL011 UART 在 EL1 确认；独立的 C++ 桥在 shell 之前初始化 | 95% |
| **3. EFI ↔ C++ 内核桥** | 独立的 C++ (`-ffreestanding -fno-exceptions`) 编译为 BOOTAA64.EFI；从真实的 QEMU 调用横幅 + shell | 90% |
| **4. CanonFS 安装** | 内存驱动程序在启动时始终在线；持久驱动程序通过 `T81_CANONFS_ROOT` 激活 | 80% |
| **5. Shell /交互式提示** | 串行上的行缓冲 `t81>` shell； `help` / `version` / `status` / `policy` 命令 | 95% |
| **6。内核事件循环** | 优先调度（故障→中断→寻呼机→调度程序tick），WFI空闲 | 100% |
| | **全面的** | **~93%** |

**当前状态：** BOOTAA64.EFI 二进制文件是一个三阶段映像。第 1 阶段 (EFI) 打印 ConOut 横幅并调用 `ExitBootServices`。第 2 阶段（裸机 C）确认 EL1 PL011 MMIO 访问。第 3 阶段（独立的 C++ 桥）打印治理横幅并运行交互式 `t81>` shell - 全部编译成单个 PE32+ 二进制文件，没有托管 C++ 运行时。 Linux QEMU 上预期的串行序列运行：

````文本
Axion QEMU AArch64 EDK2 slice6

[axion] 裸机EL1内核入门
[axion] ExitBootServices 完成；移交给 C++ 内核

T81 -- 人工智能三元操作系统
  ===========================

[axion] 策略引擎：准备就绪
[axion] canonfs：已安装（内存中）
[axion] 内核线程 tid=1：正在运行

t81>
````

**剩下的干净启动：** Virtio-blk MMIO 驱动程序用于裸机上的持久 CanonFS （因此 `T81_CANONFS_ROOT` 在 QEMU 中后面有一个真正的块设备），并将托管的 `KernelRuntimeState` 事件循环（调度程序、寻呼机、GICv3 中断）连接到独立桥路径中，以便 `status` 显示实时计数器。

启动脚本、磁盘映像和捕获的串行输出位于 [`drivers/qemu/`](drivers/qemu/) 中：

- [`drivers/qemu/scripts/launch_production.sh`](drivers/qemu/scripts/launch_production.sh) — 在 QEMU 中启动镜像
- [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt) — 最近运行中确认的序列序列
- [`drivers/qemu/docs/QEMU_TESTING_RESULTS.md`](drivers/qemu/docs/QEMU_TESTING_RESULTS.md) — 完整启动测试报告

[`qemu-boot`](.github/workflows/qemu-boot.yml) CI 工作流程构建 EFI 二进制文件，组装 FAT32 GPT 映像，在每次触及 `userland/experimental/` 或 `drivers/qemu/` 的推送时在 QEMU (TCG cortex-a57 + EDK2 AArch64) 下启动它，验证所有三个阶段的所有八个启动标记，并将更新的串行日志提交回 `drivers/qemu/sample-boot-log.txt`。

---

## CLI 参考

````嘘
# 编译并执行
t81 代码构建 <file.t81> -o <file.tisc>
t81 代码运行 <file.t81|file.tisc> [--policy <apl>] [--weights-model <t81w>] [--trace]
t81 代码复制
t81 代码检查 <file.t81>

# 虚拟机检查
t81 虚拟机运行 <file.tisc>
t81 虚拟机调试 <file.tisc>
t81 虚拟机跟踪 <file.tisc>

# Axion 治理
t81 策略编译 <file.apl>
t81 策略验证 <file.apl>
t81轴子状态
t81轴子审计

# 决定论
t81 确定性 verify-run <file.tisc> # 运行两次，比较哈希值
t81 确定性哈希 <file.tisc>
t81 决定论证明 <file.tisc>

# 模型权重
t81 权重导入 <model.safetensors|model.gguf> -o model.t81w
t81 重量信息 <model.t81w>
t81 重量验证 <model.t81w>
t81 权重量化 <input> --to-gguf <out>

# TISC 字节码
t81 tisc 异常 <file.tisc>
t81 tisc 验证 <file.tisc>
t81 tisc 统计数据 <file.tisc>

# 接口
t81 studio # 人类操作员 TUI
t81 代理 # AI-native TUI
````

---

## 确定性验证

```sh
./scripts/ci/run_determinism_slice.sh
```

CI 跨平台确定性门在每次推送到 `main` 时按每日计划运行。经过验证的确定性表面上的任何散列分歧都会阻止合并。

---

## 文档

| 话题 | 地点 |
| :--- | :--- |
| 入门 (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| 入门（人工智能） | `docs/user-guide/getting-started/ai-quickstart.md` |
| 途易指南 | `docs/user-guide/how-to/tui-guide.md` |
| 指令集规范 | `spec/tisc-spec.md` |
| Axion 政策手册 | `docs/user-guide/tutorials/axion-policy-manual.md` |
| T81Lang 标准库参考 | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| 架构概述 | `docs/architecture/OVERVIEW.md` |
| 治理章程 | `docs/governance/README.md` |
| 项目控制中心 | `docs/status/PROJECT_CONTROL_CENTER.md` |
| 推理基准测试结果 | [`benchmarks/results/inference_comparison.md`](benchmarks/results/inference_comparison.md) |

---

## 治理

T81 Foundation 在 **持续治理 (C2)** 模式下运营。所有贡献必须保持：

- **确定性执行奇偶校验** - 跟踪哈希值在支持的平台上匹配
- **架构一致性** - 确定性表面的更改需要正式审查
- **规范权威** — `spec/` > `docs/architecture/` > `docs/`；冻结表面需要主要版本凹凸

确定性表面注册表在 `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` 中定义。 RFC-0048 中定义了表面边界分类（DCP/受控非 DCP/实验/范围外）。

---

## 三元优势

虽然现代二进制硬件针对通用计算进行了高度优化，但**平衡三元** ({−1, 0, +1}) 具有在确定性执行、受控人工智能推理和低复杂性神经工作负载方面特别重要的结构属性。

### 1. O(1) 否定 — 零进位传播

二进制补码否定是按位 NOT 后跟 +1，这可以触发长进位链。平衡三元取反翻转 +1 ↔ −1 并保持 0 不变 - **无进位，常数时间**。

测量：PackedCell 求反在最新的 x86_64 硬件上达到 **~49.9 G-ops/s**，比优化的 64 位整数求反快 **~10.9 倍**（在 Linux x86\_64 和 macOS ARM64 上验证）。

### 2. 优越的基数经济性

信息论最优基数是 *e ≈ 2.718*。三进制（基数 3）比二进制（基数 2）更接近，每个 trit 提供 **~1.585 位信息** (log23)。每个数字的熵更高，对称范围更紧凑——对于权重、嵌入和稀疏张量特别有用。

### 3. 固有的位精确决定论

IEEE 754 受到特定于平台的舍入模式、关联性差异和非正规处理的影响。平衡三元在零附近对称：舍入是没有方向偏差的截断。每个执行路径都会在受支持的平台上生成**相同的 CanonHash81 跟踪哈希值**。

### 4.无乘法神经推理

三元权重 {−1, 0, +1} 将点积减少为条件加/减 - 不需要乘法单元。结合六个 TISC 推理操作码：

- 与 FP16/FP32 基线相比，能耗减少 15–60 倍
- 4–90 倍的吞吐量增益，具有相当的精度
- 与 BitNet b1.58、xTern 和 2024–2026 三元变压器研究保持一致

T81 三元权重 (T81WTN) 格式和 `t81 weights import` 使该堆栈现在可以投入生产。

### 5. Trit 级治理挂钩

由于 TISC ISA 是三元本机的，因此 Axion 内核可以在出现任何副作用之前以 **trit 级粒度**拦截和审核状态转换。这使得失败关闭的策略执行、细粒度的道德门和确定性的审计跟踪成为可能，这些从根本上比黑盒二进制执行更容易检查。

---

## 许可证

## 许可证

Apache License 2.0.

---

<details>
<summary>诚实的引导说明（2026 年 3 月）</summary>

T81 被设计为具有自己的 ISA 和内核的独立操作系统，但尚不存在本机三元硬件。当前预览版通过二进制文件、Docker 或 QEMU 在 Linux/macOS/Windows 上作为来宾层运行。

这是临时的脚手架——就像早期 Linux 在真正的硬件之前在模拟器上运行一样。裸机启动处于 Alpha 阶段；目标是最终完全摆脱主机操作系统的依赖。

感谢您阅读本文。

</details>
