<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation — 确定性三元计算堆栈

![Release](https://img.shields.io/badge/release-v1.6.1--Stable-blue)
![Tests](https://img.shields.io/badge/tests-368%2F368_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.2.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

**T81 Foundation**是一个基于base-e计算的理论效率的确定性计算堆栈，基于**平衡三元算术**（{-1,0,+1}）构建，具有涵盖指令集、虚拟机、语言编译器和AI推理环境的全链治理模型。

该堆栈提供：

- **位精确再现性** — 每个执行路径在支持的平台上都会生成相同的跟踪哈希
- **受控人工智能推理** — Axion 策略引擎在产生副作用之前拦截并审核每个特权操作
- **内容寻址来源** - CanonFS 不变地记录所有工件、模型权重和运行时状态
- **确定性并行执行** — DPE 任务图模型 (RFC-DPE-0002) 支持并发 TISC 工作负载和纪元提交的输出

---

## 项目状态 — 2026 年 3 月

**阶段：积极开发** — v1.6.0-稳定版； 368/368测试通过；在 Linux x86\_64 + macOS ARM64 上验证了跨平台确定性。

| 成分 | 到期 | 笔记 |
| :--- | :--- | :--- |
| **TISC一** | ❄️冷冻 | v1.2.0；操作码语义在 v1.x 下不可变；自 v1.1 以来的 12 个新操作码：`AgentInvoke` (RFC-0015)、6 个三元本地推理 (RFC-0034)、3 个 FFI (RFC-00B8)、2 个网格加密 (RFC-0038)、1 个 KEM 环 (RFC-0039) |
| **数据类型** | ❄️冷冻 | BigInt、Float、Complex、Map、Set — 位稳定编码； 2026-02-27 审核清洁 |
| **T81VM** | ✅ 稳定 | 完整的 TISC v1.2 调度；  `AgentInvoke` + 三元原生推理 + FFI + 格密码 + NTRU-KEM 操作码； 368/368 测试 |
| **T81Lang** | ✅ 稳定 | 规范 v1.3 稳定；  `agent` / `behavior` (RFC-0015);  `foreign {}` FFI（RFC-0036）；  `std.tnn.*` TNN 标准库（RFC-0037）；  `std.crypto.*` 格子加密 + NTRU-KEM (RFC-0038/0039)；整个上下文标识符支持 |
| **Axion 治理内核** | ✅ 稳定 | 满足 P4 安全和 P5 特权指令； AX-M6 规范原因字符串；每个 `AgentInvoke` + `TACT` 激活门都会发出审核事件 |
| **三元本地推理** | ✅ 已接受 | RFC-0034 + RFC-0037：`TWMATMUL`、`TQUANT`、`TATTN`、`TWEMBED`、`TERNACCUM`、`TACT`；  `std.tnn.*` T81Lang stdlib（6 个内置 → TISC 操作）；无乘法推理； T81WTN重量格式； 13/13 测试 |
| **格密码学** | ✅ 已接受 | RFC-0038+0039：`POLYMUL`、`POLYMOD`、`TVecSub`； Z\[x\]/(x^n+1) 上的满环 {+,−,×,mod}；  `std.crypto.{polyadd,polysub,polymul,polymod,ntru_encrypt,ntru_decrypt}`; 37/37 测试 |
| **受监管的 FFI** | ✅ 已接受 | RFC-00B8 + RFC-0036：`FFIDispatcher`、`FFILibraryRegistry`、3 个 VM 操作码；  `foreign [policy] { fn … }` T81Lang 语法；  `foreign.<name>(args)`→`FFI_CALL`; 9/9 交流测试 |
| **TUI 前端** | ✅ 已接受 | `t81 studio`（人工操作员）+ `t81 agent`（人工智能原生）； FTXUI v5.0.0； RFC-0033 已接受 |
| **T81图** | ✅ 测试版 | VM 操作码降低 + lang 端有线序列化； DCP验证完成； 6/6 测试 |
| **DPE（并行执行）** | ✅ 已接受 | RFC-DPE-0001–0009 全部接受；任务图、纪元历史环、纪元审计事件、超时完全实现 |
| **认知层** | ✅ 已接受 | Tier4 认知 (RFC-0021)：`Tier4Loop`、`SelfModel`（81 入口环）、`RecursiveImprovementBounds`、`TierAwarePlanner`； 4个测试套件通过 |
| **基准套件** | ✅ 已接受 | RFC-00A2：VM 吞吐量 + CanonHash81 确定性验证（所有运行均为 `score=1.0`）；  `t81 internal benchmark` |
| **跨平台确定性 CI** | ✅ 已接受 | 每日 GitHub Actions 工作流程比较 Linux x86\_64 (gcc-14) 和 macOS ARM64 (clang) 上的 T81Lang 字节码哈希值；可公开审计的证据记录 |
| **Axion 操作系统内核** | 🔬 实验性的 | TernaryOS：寻呼机、调度程序、IPC、中断框架、QEMU x86\_64 EFI 通道可操作； 9/9 三元操作系统测试通过 |

---

## 建筑学

```
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang Compiler                                           │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
│  agent/behavior (RFC-0015)  ·  foreign {} (RFC-0036)        │
├─────────────────────────────────────────────────────────────┤
│  Axion Governance Kernel                                    │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81 Virtual Machine         │  DPE Task Graph Runtime      │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA v1.2  ❄️ Frozen  +  Data Types  ❄️ Frozen         │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  FFIDispatcher · FFILibraryRegistry                         │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS (Axion OS Kernel) · Cognitive Tiers
```

### 关键部件

**TISC ISA v1.2** — 三元指令集架构。在 v1.x 下冻结；整个堆栈的不可变执行合约。 v1.2 添加了 9 个操作码：`AgentInvoke` (RFC-0015)、六个三元本机推理操作 (RFC-0034) 和三个受治理的 FFI 操作 (RFC-00B8)。

**T81VM** — 确定性 TISC 解释器。保证跨平台比特​​相同的输出； Axion 预调度隔离将治理挂钩保持在热执行路径之外。完整的 TISC v1.2 调度，包括三元本地推理和 FFI。

**Axion 治理内核** — 在产生任何副作用之前拦截 `AXREAD` 、 `AXSET` 、 `AXVERIFY` 、AI 操作码和 FFI 调用的策略引擎。策略解析失败时失败关闭。 2026 年 3 月 15 日获得稳定认证，54/54 测试通过。

**CanonFS** — 内容寻址文件系统。将所有代码对象、模型权重和运行时工件存储为不可变的哈希标识的 blob。为确定性审计提供出处。

**T81Lang** — 针对 TISC 字节码的高级语言。原生类型：`BigInt`、`Fraction`、`Float`、`Complex`、`Tensor`、`Map`、`Set`。一流的 `agent { behavior }` 声明通过 Axion 审核 (RFC-0015) 编译为 `AGENT_INVOKE`。  `foreign [policy] { fn … }` 块声明通过 `FFI_CALL` (RFC-0036) 调用的受控外部函数。  `agent` 、 `behavior` 和 `foreign` 可用作所有表达式和绑定位置中的上下文标识符。编译器管道：词法分析器 → 解析器 → 类型化 AST → 语义分析 → IR 生成。

**三元原生推理 (RFC-0034)** — 使用平衡三元权重 {−1, 0, +1} 进行无乘法 AI 推理的 6 个 TISC 操作码：`TWMATMUL` (matmul)、`TQUANT`（量化为 trit）、`TATTN`（三元注意力）、`TWEMBED`（权重嵌入）、`TERNACCUM`（标量点）产品），`TACT`（使用 Axion 天花板门激活）。 T81WTN 重量格式。 T81Lang `foreign {}` 前端通过 RFC-0036 完成。

**受控 FFI (RFC-00B8 + RFC-0036)** — 全栈受控外部函数接口。 VM层（RFC-00B8阶段1）：`FFIDispatcher`在任何外部调用之前强制执行策略检查、资源配额和审计跟踪；  `FFILibraryRegistry` 通过名称和版本哈希跟踪注册库；三个 VM 操作码（ `FFICall` 、 `FFIRegister` 、 `FFIPolicySet` ）。语言层（RFC-0036）：`foreign deterministic { fn sin(x: T81Float) -> T81Float; }`声明签名；  调用点的 `foreign.sin(angle)` 降低到 `FFI_CALL` ，函数名称携带在 `text_literal` 中。九项验收测试通过。

**TUI 前端** — 两个基于 FTXUI v5.0.0 构建的互补终端接口：

- `t81 studio` — 导航侧边栏、CanonFS 浏览器、Axion 仪表板、确定性跟踪可视化工具、命令调色板 ( `Ctrl+P` )
- `t81 agent` — 持久 JSONL 会话、斜杠命令（`/compile`、`/run`、`/hash`、`/allow`、`/infer`、`/trits`、...）、trit 概率条

**DPE（确定性并行执行）** - 冻结 TISC ISA 上的任务图模型。任务声明不可变的输入和缓冲的输出区域；虚拟机在纪元结束时自动提交所有写入。不需要新的操作码。

---

## 快速入门

```bash
# Clone and configure
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the full test suite
ctest --test-dir build --output-on-failure

# Launch the human operator TUI
./build/t81 studio

# Launch the AI-native TUI
./build/t81 agent

# Compile a T81Lang program
./build/t81 code build examples/hello.t81 -o hello.tisc

# Execute with Axion governance
./build/t81 vm run hello.tisc
```

可选的构建标志：

| 旗帜 | 默认 | 目的 |
| :--- | :--- | :--- |
| `T81_BUILD_TUI` | `ON` | 基于 FTXUI 的 TUI 前端 |
| `T81_BUILD_TESTS` | `ON` | 完整的测试套件 |
| `T81_ENABLE_ASAN` | `OFF` | 地址消毒剂 |
| `T81_ENABLE_UBSAN` | `OFF` | UB消毒剂 |
| `T81_ENABLE_LLAMA_CPP` | `OFF` | 受控 llama.cpp 推理适配器 |
| `T81_WARN_STRICT` | `OFF` | 严格警告扫描模式（`warn-strict`预置位使用） |

**推送前警告扫描** — 镜像 Windows CI 强制执行的 `-Wswitch` 、 `-Wunused-variable` 和 `-Wunused-function` 检查，在大约 2 分钟内捕获本地问题，而不是等待完整的矩阵：

```bash
cmake --preset warn-strict
cmake --build build-warn-strict 2>&1 | head -40
```

---

## 确定性验证

每个版本都经过比特精确跨平台再现性验证。

```bash
./scripts/ci/run_determinism_slice.sh
```

已验证平台：**Linux x86_64**、**macOS ARM64**。虚拟机跟踪哈希中的任何分歧都是一个严重缺陷。

---

## 文档

| 话题 | 地点 |
| :--- | :--- |
| 入门 (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| 入门（人工智能） | `docs/user-guide/getting-started/ai-quickstart.md` |
| 途易指南 | `docs/user-guide/how-to/tui-guide.md` |
| 指令集规范 | `spec/tisc-spec.md` |
| 轴子政策手册 | `docs/user-guide/tutorials/axion-policy-manual.md` |
| T81Lang 标准库参考 | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| 架构概述 | `docs/architecture/OVERVIEW.md` |
| 治理章程 | `docs/governance/README.md` |
| 项目控制中心 | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## 路线图

| 里程碑 | 目标 | 描述 |
| :--- | :--- | :--- |
| C2 月结 | 2026-03-31 | 治理账本审计；飞行前通行证 2026-03-10 |
| Axion 稳定版促销 | ✅ **2026-03-15 完成** | 实施 AX-M6 规范原因字符串； 54/54 测试通过；生产就绪 |
| T81Graph Beta 促销 | ✅ **2026-03-15 完成** | VM操作码降低完成； DCP验证； 6/6 测试通过 |
| RFC-00B5 中断策略 | ✅ **2026-03-16 完成** | 集成受控事件中断模型；切片 26-28 完整 |
| RFC-0034 三元本机推理 | ✅ **2026-03-16 完成** | 6 个新的 TISC 操作码；无乘法推理； TACT激活-天花板门； 5/5 一致性测试 |
| RFC-00B8 受监管的 FFI（第 1 阶段） | ✅ **2026-03-16 完成** | FFI 调度程序 + 库注册表； 3 个 VM 操作码；治理管道；审计追踪 |
| 跨平台确定性 CI | ✅ **2026-03-16 完成** | 每日 GitHub Actions 工作流程； Linux x86\_64 + macOS ARM64 哈希比较；公开证据记录 |
| RFC-0036 T81Lang FFI 语法 | ✅ **2026-03-16 完成** | `foreign [policy] {}` 语法；  `foreign.<name>(args)`→`FFI_CALL`; 9/9 交流测试；将 RFC-0034 + RFC-00B8 VM 工作连接到 T81Lang 前端 |
| 第二阶段：验证平台 | ✅ **2026-03-16 实现** | 所有实施目标均已完成；跟踪重放调试器、跨平台 CI、365/365 测试、FFI 前端 — 外部可重现堆栈 |
| RFC-0037 TNN 标准库 | ✅ **2026-03-16 完成** | `std.tnn.*` T81Lang 内置函数（6 个函数 → RFC-0034 TISC ops）； 13/13测试；从源到虚拟机的全栈无乘法推理 |
| RFC-0038 莱格加密 | ✅ **2026-03-16 完成** | `POLYMUL` / `POLYMOD` TISC 操作码；  `std.crypto.polymul/polymod` 个内置函数；负循环多项式乘以 {−1,0,+1}； T81BigInt-精确； 13/13 测试 |
| T81Lang规格升级（v1.3） | ✅ **2026-03-16 完成** | RFC-0036/0037/0038晋升为规范规范； §5.17 未存根；添加了§5.18/5.19；操作码注册表更新至 205 项 |
| RFC-0039 NTRU-KEM | ✅ **2026-03-16 完成** | `TVecSub` 操作码；  `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`； C++ KEM 数学层； 24/24 测试； Z\[x\]/(x^n+1) 上的满环 {+,−,×,mod} |
| TernaryOS裸机启动 | 待定 | x86\_64 QEMU主机执行+CanonFS证据返回 |

---

## 治理

T81 基金会在 **持续治理 (C2)** 模式下运营。所有贡献必须保持：

- **确定性执行奇偶校验** - 跟踪哈希必须在支持的平台上匹配
- **架构一致性**——触及确定性表面的变更需要正式审查
- **再现性保证** — DCP 表面中没有浮点或特定于平台的非确定性

确定性表面在 `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` 中定义。对冻结表面（TISC ISA、数据类型）的更改需要主要版本更新。

> **边界注释：** 实验表面（认知层、分布式、Trace-JIT、TernaryOS、llama.cpp 适配器）受非 DCP 管理，不得作为经过验证的确定性组件呈现。

---

## 三元优势

虽然现代二进制硬件经过高度优化，但**T81 Foundation** 利用**平衡三元 ({-1, 0, +1})** 的独特数学属性来实现二进制无法比拟的结构效率。

### 1. $O(1)$ 计算对称性

在二进制补码中，求反数是一种不对称运算 (NOT + 1)，需要进位传播。在 T81 中，否定是一个简单的 trit-flip，具有**零进位开销**。

* **性能：** T81 求反吞吐量达到 **~46.6 G-ops/s**（通过 `PackedCell`），比优化的 64 位二进制求反性能高出 **10.4 倍**。

### 2. 优越的基数经济性

根据数字系统最有效的基数为 $e \approx 2.718$ 的定理，三进制（基数 3）在数学上比二进制（基数 2）更有效。

* **信息密度：** T81 实现了 **1.58 位/trit** 的理论密度。这意味着每个时钟周期的熵更高，并减少了大规模坐标系统和神经权重的存储占用空间。

### 3. 位精确决定论

二进制浮点运算 (IEEE 754) 通常会受到特定于平台的舍入不确定性的影响。 T81的平衡算术提供：

* **固有对称性：** 通过简单截断来执行舍入，因为系统自然以零为中心。
* **跟踪奇偶校验：** 在所有测试平台（Linux x86_64、macOS ARM64）上实现 100%“往返精度”，虚拟机跟踪哈希值的偏差为零。

### 4.直接治理挂钩

由于 TISC ISA 是三元原生的，**Axion 治理内核** 可以以更高的粒度审核状态转换。人工智能推理操作可以在任何副作用发生之前在“trit-level”被拦截，从而实现“故障关闭”安全模型，这在标准“黑盒”二进制执行中在架构上是不可能的。

---

## 战略应用

T81 堆栈的结构优势（特别是 **10.4 倍的否定吞吐量**和 **1.58 位/trit 密度**）能够解决传统二进制瓶颈：

---

---

# T81：完整的系统与子系统报告

## 1. 执行摘要

T81 Foundation 并非单一项目。它是一个多层的代码库，结合了三元数据模型、ISA、VM、语言/编译器表面、治理层、内容寻址存储、基准测试和 CI 机制、实验性的 OS/内核开发，以及一套庞大的治理/文档体系。仅仅是仓库的根目录就展示了巨大的系统规模：`.github`，`benchmarks`，`book`，`contracts`，`core`，`docs`，`experimental`，`kernel`，`lang`，`runtime`，`spec`，`src`，`tests`，`tooling`，`tools`，加上 `internal`，`legacy`，`notebooks`，`pdf`，以及多语言/面向公众的资产。该项目包含 3,636 次提交，主要由 C++ 编写，包含少量 Python/CMake/Shell/C 组件。 ([GitHub][1])

最强的技术核心是围绕确定性执行栈的：数据类型、TISC、T81VM 的非 JIT 解释器路径，以及相关的确定性注册表/DCP 边界。这些区域在“确定性核心配置文件” (Deterministic Core Profile) 中被明确命名，与验证工件绑定，并有 CI 检查和具体的测试路径支撑。架构概览也清晰地将冻结/稳定的核心与实验性的外围分离开来。 ([GitHub][2])

最大的结构弱点在于权限表面状态的语无伦次。代码库同时呈现了多种冲突的成熟度/版本叙述：根 README 表示“v1.9.0-Stable; 369/369 tests”，项目控制中心表示“v1.4.1-Stable; 363/363 tests”，`CMakeLists.txt` 显示版本为 `1.3.6`，TISC 规范为“Version 1.1 — Stable”，而 README 描述的是“TISC ISA v1.2”，且 Axion 内核在实现矩阵中显示为“Stable”，在规范和实验性 OS 进度日志中却显示为“Alpha”。这并未否定实现的价值，但从根本上降低了审计的可靠性和外部可信度。 ([GitHub][1])

## 2. 评估方法

本次评估基于六个证据流：仓库根目录的结构和历史；`spec/` 中的规范性文档；`docs/` 中的架构/治理/状态文档；构建和 CI 表面，如 `CMakeLists.txt` 和 `.github/workflows/ci.yml`；成熟度仪表盘，如 `PROJECT_CONTROL_CENTER.md` 和 `IMPLEMENTATION_MATRIX.md`；以及位于 `experimental/ternaryos/docs/PROGRESS.md` 下的实验性操作系统的进度日志。仓库自身的权威顺序是明确的：`/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`。 ([GitHub][1])

因此，这是一次结构/规范/实现表面审计，而不是对每个 C++ 文件的完整逐行源代码验证。在仓库自身声明了实现状态、测试或成熟度的地方，我将这些视为声明，除非得到相邻证据的支持，例如 CI 检查、显式的测试路径或文档间的交叉一致性。在仓库自身包含矛盾的地方，我将其视为治理偏差的证据。 ([GitHub][3])

## 3. 系统体系图

### 架构基础

目的：定义有界架构、权威模型和确定性核心边界。主要位置：`docs/architecture/OVERVIEW.md`，`docs/product/DETERMINISTIC_CORE_PROFILE.md`，`docs/governance/DETERMINISM_SURFACE_REGISTRY.md`，`spec/`。成熟度：相对较高，作为一种文档控制表面。耦合：一切都依赖于它。主要风险：这些文件与面向公众的顶层信息之间的状态漂移。 ([GitHub][2])

### 数据模型 / 数值模型

目的：规范的三元原生数据类型和确定性编码。主要位置：`core/types/`，`spec/t81-data-types.md`，在 DCP/注册表中列出的相关测试。成熟度：属于最可靠的领域；在实现矩阵和 DCP 中被视为“已冻结/已验证” (Frozen/Verified)。主要风险：浮点数的范围被小心地限制了，但公众的信息可能会模糊这些限制。 ([GitHub][4])

### ISA / 虚拟机 / 运行时

目的：冻结的指令语义和确定性的执行环境。主要位置：`core/isa/`，`core/vm/`，`runtime/`，`spec/tisc-spec.md`，`spec/t81vm-spec.md`。成熟度：解释器路径是核心的操作基底；尽管仪表盘将其提升为“稳定” (Stable)，VM 规范仍处于 Beta 状态。主要风险：规范/仪表盘不一致以及未完成的 JIT 等效性。 ([GitHub][2])

### 语言 / 编译器表面

目的：专门编译为 TISC 的确定性高级语言。主要位置：`lang/`，`spec/t81lang-spec.md`，文档中命名的固定装置 (fixtures) 和复现门 (repro gates)。成熟度：比许多实验语言仓库更强，因为它具有规范的语法、编译器管道描述和明确的确定性警告。主要风险：语言成熟度声明强于此处直接检查的实现证据。 ([GitHub][5])

### 治理 / 策略 / Axion

目的：监督特权指令、策略判定和执行可见性。主要位置：`kernel/axion/`，`spec/axion-kernel.md`，治理文档，与跟踪相关的文档。成熟度：混合的。该架构足够真实，可以塑造 VM 和策略论述，但规范中的几个核心管理声明明确表示只是部分实现。主要风险：治理修辞在某些领域超出了当前的执行力度。 ([GitHub][6])

### 存储 / CanonFS / 对象模型

目的：内容寻址的持久性和溯源。主要位置：`src/canonfs/`，`include/t81/canonfs/`，补充规范和 Axion OS 进度说明。成熟度：在架构文档中受到稳定限制；更宏大的客户机/启动/持久性场景在操作系统轨道中仍处于实验阶段。主要风险：混合了稳定核心存储溯源声明与更广泛的原型存储/启动声明。 ([GitHub][2])

### 内核 / 操作系统 / Axion 操作系统表面

目的：操作系统基底、MMU、调度器、IPC、设备/HAL 接口、服务运行时、启动通道。主要位置：`experimental/ternaryos/`，`kernel/`，操作系统进度文档。成熟度：高级托管原型，尚未成为完整的操作系统。主要风险：Axion 作为治理内核和 Axion 作为操作系统之间的命名混淆。 ([GitHub][7])

### 人工智能 / 认知 / 推理表面

目的：三元原生推理、认知层、受管代理/运行时表面、FFI、一些 AI 实验边界。主要位置：`experiments/ ai`，`experimental/`，README/规范中引用的基于 RFC 的功能。成熟度：混合的；推理操作码工作似乎已经实质性地实现，而认知层和更广泛的 AGI/治理目标仍然部分处于推测阶段，或者明确处于非 DCP 范围。主要风险：过度扩张。 ([GitHub][1])

### 测试 / 基准测试 / CI 强制执行

目的：强制执行结构完整性、确定性声明、规范/文档对齐和基准门限。主要位置：`.github/workflows`，`benchmarks/`，`tests/`，`scripts/ci/`，`scripts/governance/`。成熟度：相对于项目阶段而言很强。主要风险：CI 广度很大，但某些工作是信息性的，并且公共数据在各文档间不一致。 ([GitHub][1])

### 文档 / 公共沟通

目的：规范、审计、仪表盘、书籍、多语言 README 表面。主要位置：`docs/`，`book/`，README 翻译。成熟度：数量非常高，同步完整性好坏参半。主要风险：文档蔓延和竞争性的权威层级。 ([GitHub][1])

### 遗留 / 内部 / 笔记 / PDF / 存档

目的：历史、支持工件、探索性材料、生成的材料。主要位置：`legacy`，`internal`，`notebooks`，`pdf`，`artifacts/archive`。成熟度：设计上不明确。主要风险：死亡的表面、不明确的支持状态、贡献者的困惑。 ([GitHub][1])

## 4. 详细的子系统清单

### T81 数据类型

目的：规范的数值和集合语义。位置：`core/types/`，`spec/t81-data-types.md`，在 DCP/注册表中命名的测试。状态：已实现，已验证，并被视为已冻结。验证证据：`v1_canonical_numeric_contract_test.cpp`，`tisc_binary_io_determinism_test.cpp`，以及实现矩阵中的审计说明。确定性相关性：基础。治理相关性：高，因为规范化和策略重放依赖于稳定的表示。主要担忧：浮点范围必须保持有限。 ([GitHub][4])

### TISC ISA

目的：不可变的执行契约。位置：`core/isa/`，VM 解码/调度，`spec/tisc-spec.md`。状态：核心运行，但状态/版本报告不一致：实现矩阵将其视为冻结/已验证，README 提到 v1.2，而规范页面为“Version 1.1 — Stable”，并且 CI 仍参考“验证 TISC v1.1.0 冻结完整性”。确定性相关性：最高。治理相关性：Axion 的可见性在规范中是强制性的。主要担忧：冻结完整性受到强有力管理，但版本簿记呈现不连贯。 ([GitHub][4])

### T81VM 解释器

目的：TISC 的确定性执行环境。位置：`core/vm/`，`spec/t81vm-spec.md`。状态：实际上是核心组件，并被仪表板视为稳定 (Stable)，但在规范规范中仍为 Beta。验证证据：虚拟机跟踪和确定性属性测试，DCP 包含，一致性提及。确定性相关性：最高。治理相关性：明确的 Axion 集成。主要担忧：未公开公共 `get_execution_mode()`；调度事件尚不是一等公民的跟踪记录；跨架构的浮点数保证被明确限制。 ([GitHub][8])

### T81Lang

目的：针对 TISC 的高级确定性语言。位置：`lang/`，`spec/t81lang-spec.md`，注册表中命名的复现脚本/固件。状态：通过文档和声明的升级状态来看是更强大的子系统之一。验证证据：确定性固件、复现门限、完整的语法、管道描述。确定性相关性：高，但明确声明了主机相关的超越函数/浮点行为的注意事项。治理相关性：层级感知、纯度/效果、Axion 可见性。主要担忧：成熟度的声明强于从直接检查的实现面所能确认的证据。 ([GitHub][5])

### Axion 治理内核

目的：执行上方的策略和监督层。位置：`kernel/axion/`，`spec/axion-kernel.md`。状态：部分真实，部分属于理想目标。实现矩阵将其提升为稳定 (Stable) 并通过 54/54 个测试，但规范规范仍然说 Alpha，并且明确承认确定性管理 (stewardship) 只实现了部分，并且复杂性指标也不完整。确定性相关性：高。治理相关性：这是治理核心。主要担忧：架构具有重要意义，但“最终仲裁者”的措辞超出了目前已实现的检测能力。 ([GitHub][4])

### CanonFS

目的：内容寻址的不可变来源和持久性。位置：`src/canonfs/`，`include/t81/canonfs/`，补充文档/规范。状态：在架构概述和与 DCP 相关的文档中处于稳定限制状态；更高级的持久性和客户机引导启动 (guest-bootstrap) 声明属于实验性 OS 路径。验证证据：引用的源路径和 OS 阶段测试。主要担忧：该仓库将稳定核心溯源声明与仍然明显处于原型阶段的更广泛客户机/启动持久性工作混为一谈。 ([GitHub][2])

### 确定性核心配置文件 (DCP) / 注册表

目的：定义认证边界和经过验证的确定性表面。位置：`docs/product/DETERMINISTIC_CORE_PROFILE.md`，`docs/governance/DETERMINISM_SURFACE_REGISTRY.md`。状态：实际的治理机制，而不仅仅是品牌宣传。验证证据：显式的表面/测试/CI 映射。主要担忧：DCP 边界是良好的工程实践，但更广泛的仓库叙述有时会模糊它。 ([GitHub][9])

### CI / 治理脚本

目的：执行结构、文档/规范一致性、冻结完整性、工作流卫生、治理检查、DCP 对齐和基准门。位置：`.github/workflows/ci.yml`，`scripts/ci/`，`scripts/governance/`。状态：对于实验性系统仓库来说异常强大。主要担忧：广度令人印象深刻，但信息性工作不等于生产证据，并且检查的 CI 文件本身只是工作流资产的一部分。 ([GitHub][3])

### 基准测试 (Benchmarks)

目的：性能和工作负载门槛。位置：`benchmarks/`，CI 中的基准门槛，README 基准声明。状态：存在并且能够对一些工作进行把关，但基准测试学科尚未完全与稳定版本的报告统一。主要担忧：营销风格的 README 部分中的性能声明比此处检查到的确凿证据更宽泛。 ([GitHub][1])

### Axion 操作系统 / TernaryOS

目的：带有 MMU、调度程序、IPC、设备、分页器、服务运行时、启动通道的托管内核/操作系统底座。位置：`experimental/ternaryos/`，`kernel/`，操作系统进度日志。状态：高级托管原型，具有大量逐片实现的证据。主要担忧：尽管具有真正的工程深度，但它显然仍是实验性的、托管的，并且部分建立在 QEMU/VirtualBox/开发通道工作流之上，而不是一个成熟的独立操作系统。 ([GitHub][7])

### TUI / CLI / 工具

目的：操作员和代理前端、开发人员工作流、示例。位置：`tools/`，`tooling/`，`examples/`，`T81_BUILD_TUI` 选项，README 文档。状态：实现程度足以发布和被描述，但并非系统可信度的核心。主要担忧：UI/工具的广度可能会超过核心架构边界的稳定速度。 ([GitHub][10])

### 实验性 / 分布式 / 认知层 / 人工智能

目的：非 DCP 扩展路径。位置：`experimental/`，`experiments/ ai`，`runtime/jit/`，DCP/注册表中的分布式和层级参考。状态：根据仓库自身的定义，属于研究阶段或存根。主要担忧：这些表面增加范围的速度快于增加操作证明的速度。 ([GitHub][1])

## 5. 架构一致性评估

存在真正的分层模型。架构概述呈现了从 T81Lang 到 TISC、到 T81VM、到 Axion、到 CanonFS 的连贯链条，实验层被明确显示为可选的/非 DCP 的。DCP 和确定性注册表通过将保证缩小到指定的已验证表面来强化了这一点。这是架构自我意识的强烈标志。 ([GitHub][2])

但是，子系统边界只是部分真实的。核心执行边界似乎相当真实：数据类型、ISA、解释器和选定的确定性序列化/测试表面。在这个核心之外，边界变得模糊。"Axion" 既指稳定堆栈中的治理内核，也指实验性操作系统。T81VM 规范是 Beta 阶段，而仪表板称其为 Stable 阶段。Axion 规范是 Alpha 阶段，而实现矩阵称 Axion Kernel 为 Stable 阶段。OS 进度文档仍然使用内部名称 `ternaryos`。这不是一个微小的措辞问题；这意味着架构有多个重叠的自我描述。 ([GitHub][6])

接口部分显式部分隐式。规范在命名必需的行为、不变量和一致性程序方面做得很好。CI 还命名了对齐检查。但仍有几个关键接口被跟踪为不完整或间接：没有通过干净的公共 API 公开 VM 执行模式；调度器事件并非一等跟踪事件；主动的 Axion 非确定性检测仅实现部分。这意味着一些合约的规范比实现要好。 ([GitHub][8])

最强的缝合点是 DCP 边界和存储库的内部授权模型。最弱的缝合点是命名/版本/状态的同步，以及从严格的确定性核心扩展到操作系统、认知、AI 和硬件的叙述。文档在仓库树和密集的仪表板生态系统中蔓延可见。 ([GitHub][1])

## 6. 确定性与再现性评估

确定性是仓库中最严肃的工程主题。它在 README 中声明，在 TISC 和 T81VM 规范中形式化，在架构概述中划定边界，在 DCP 中缩小范围，并在确定性表面注册表中枚举。该注册表非常有用，因为它区分了经过验证的表面与排除的或计划的表面。 ([GitHub][1])

其中真正强制执行的确定性：TISC 操作码语义、虚拟机解释器执行、规范数据类型编码和软件浮点 (soft-float) 确定性数学都被列为已通过指定的测试和 CI 强制执行验证。然后，DCP 将此注册表用作发布保证的大门。CI 此外还运行冻结完整性检查、架构连贯性检查和确定性声明的强制执行。 ([GitHub][11])

其中只有部分或有限的确定性：编译器字节码发射在注册表中明确表示只是“部分”；JIT 等效性只是计划中的，未被验证；网络 I/O、实时调度、软件浮点以外的硬件 FPU 行为以及性能确定性明确超出了范围。T81Lang 同样声明浮点除法和超越函数的行为可能因架构而异。这是良好的工程诚实性。 ([GitHub][11])

主要的不匹配不在于技术野心与零实现之间的对比，而在于技术野心与状态管理原则之间的对比。该代码库具有设计良好的确定性边界，但其面向公众的版本/状态页面杂音太多，足以削弱对版本发布精确性的信心。当一个项目建立在精确到位的重现性基础上时，版本的精确性也至关重要。 ([GitHub][9])

## 7. 治理、安全与策略层评估

这里的治理具有架构意义，而不仅仅是修辞意义。Axion 存在于 ISA 和 VM 的规范、架构概述、DCP 的边界以及实现矩阵中。该仓库的 CI 同样包含了多种治理和越界声明的检查，这比普通的、仅限于 README 中的治理描述要稳健得多。 ([GitHub][12])

尽管如此，规范本身显示治理层尚未完全实现。Axion 的“确定性管理” (determinism stewardship) 被明确标示为部分的；主动的不确定性检测尚未实现。复杂度的衡量标准也是部分的。规范还指出，规范的原因字符串连接 (canonical reason-string concatenation) 是一个被追踪的缺失项。因此，治理堆栈是真实的，但还不够完善，无法在没有限定条件的情况下证明该代码库提出的更为广泛的“监督智能” (supervisory intelligence) 框架。 ([GitHub][6])

最有说服力的治理机制并非来自“伦理”话语，而在于对特权操作的受限策略拦截，再加上明确的可追踪性要求。最缺乏说服力的部分在于，在基础的检测和衡量机制仍不完善的情况下，将治理话语延伸至认知层级以及高级推理监督领域。 ([GitHub][6])

## 8. 实施现状检查

当前看来确实能起作用的方面：围绕数据类型、TISC、T81VM 的解释器路径建立的确定性核心配置文件，某些特定的编译器再现性基准，可观的 CI/治理技术栈，以及足以支持具备策略意识的执行声明的 Axion 整合度。该仓库包含具体的测试用例、明确命名的工作流以及围绕该核心明确标注的稳定/冻结状态。 ([GitHub][9])

当前看来部分发挥作用的方面：Axion 更丰富的管理模型的部分内容、编译器及再现性层面的部分功能、操作系统项目中的服务/查询层，以及作为完全同步控制面的基准测试/发布规程。它们并未缺失，只是不像仓库中最宏大的论述所暗示的那样完备。 ([GitHub][6])

看似已搭建框架但尚未达到最强可操作意义上的部分：trace-JIT 等价性、分布式/认知层的不确定性、外部硬件加速器、操作系统路径中的实际硬件适配器以及更宽泛的“全球不确定性计算平台”的宏图。仓库自身将其中许多归类为实验性、计划内或不属于 DCP 的内容。 ([GitHub][9])

最成熟的区域：不确定性核心的数据类型、ISA、解释器、CI/治理框架，以及——从文档成熟度来判断——语言层面。最具抱负的区域：全方位的治理智能、分布式/认知层级、在原生硬件上的实现，以及将操作系统转变为通用内核基底。 ([GitHub][4])

## 9. 测试、验证及持续集成 (CI) 评估

在测试和持续集成 (CI) 方面的态度是一大优势。受审查的 CI 工作流涵盖了结构验证、体系结构目标检查、TISC 冻结完整性校验、AI 实验的边界控制、体系结构的一致性审查、工作流的锁定与权限审计、广泛的治理规则检查、DCP 完整性校验、治理的评估指标，以及针对基准负载的控制机制。这比一般的业余系统代码库要严谨得多。 ([GitHub][3])

此外，该代码库通过将不确定性声明与具体的测试用例和脚本相关联，提供了一种便于审计的方式。在不确定性注册表及 DCP 中，采用的是明晰的路径，而非模糊的保证。这是一个恰当的模式。 ([GitHub][11])

盲区依然存在。某些 CI 任务仅具有信息提示功能，并不构成强制性的关卡。在不确定性注册表中，编译器字节码的生成仅有部分支持。虚拟机规范自身也承认，缺少直接的查询接口以及对调度轨迹的可见度不足。在各个权威文档中，基准测试的指标与报告数量并不一致。 ([GitHub][3])

## 10. 文档和规范完整性评估

文档数量非常庞大，且在工程控制方面的意图也很严肃。体系结构概述明确说明了权威顺序。实现矩阵试图将描述的偏差控制在每个子系统对应一行。项目控制中心集中管理各关卡的状态。这体现了良好的治理设计。 ([GitHub][2])

问题出在同步的完整性上。从审查的材料中至少可以发现五个主要的矛盾之处：README 版本/测试计数与项目控制中心以及 CMake 版本的差异；TISC v1.2 与 TISC 规范 v1.1；T81VM 测试版规范与稳定版仪表板；Axion Alpha 规范与稳定版实施矩阵；以及 Axion 操作系统 Alpha 版/原型版信息与其他地方提到的一些更宽泛的稳定内核表述。这些都不是表面问题。它们让新的维护者、合作伙伴或评估人员难以确定什么是具有权威性的。 ([GitHub][1])

对于新加入的技术贡献者，代码库令人印象深刻，但难以解析。`docs`、`book`、`spec`、`contracts`、`internal`、`legacy`、`pdf`、`notebooks` 以及多个仪表板和面向公众的多语言界面的存在清楚地表明，这是一个重度依赖文档的环境。如果没有强有力的同步机制，它将变成一个迷宫。 ([GitHub][1])

## 11. 内核/操作系统/Axion 评估

这项操作系统的工作，在深度上远不止一个简单的概念性说明。进度日志中详细记录了 MMU（内存管理单元）、调度与 IPC（进程间通信）、持久性存储、设备支架、分页器 ABI、中断管理切片以及引导通道验证等已实施的阶段。其中引用了数千条断言，并规划了包含具体文件和测试的分阶段路线图。这是一项货真价实的工程工作。 ([GitHub][7])

但它最好仍被归类为一种高级的宿主原型，而非完整的内核。进度日志多次描述了宿主模拟路径、QEMU 开发者通道、VirtualBox 脚手架、回收工件的交接捆绑，以及公开的实机硬件适配器开发工作。其架构正通过阶段性切片和模拟支持的验证稳步推进，这值得尊重，但这与一个成熟的独立操作系统还不是同一概念。 ([GitHub][7])

在命名上的分歧同样值得注意。在进度日志中，“Axion”指的是操作系统，而规范将“Axion 内核”定义为生态系统的监控智能层。这两个概念互相关联，但不完全等同。若不划定更严谨的语义界限，这种命名冲突将继续引发混淆。 ([GitHub][7])

结论：这目前是一个先进的架构原型 / 部分的操作系统基础，具备真实的子系统，但还未成为通用型或生产级别的操作系统。 ([GitHub][7])

## 12. 语言 / 虚拟机 / ISA 技术栈评估

TISC 是其中最稳固的概念支撑点。它具有规范性、以版本冻结为导向，并明确界定了确定性执行、零未定义行为、对 Axion 的可见性，以及与数据类型、虚拟机（VM）和语言层的兼容性。 ([GitHub][12])

T81VM 是运行的核心。解释器路径无疑位于确定性核心（DCP）的中心，并且 DCP 将 JIT（即时编译）排除在外，直到其等效性得到证实。规范同样表现出了令人赞赏的诚实态度，列出了当前尚未完成的部分，而非假装它们已经就绪。 ([GitHub][8])

T81Lang 比众多实验性语言更成熟，因为它具备标准语法、类型/作用机制、清晰的确定性说明，以及完整的编译流程。然而，这也是项目野心最易超越现实的一环。“稳定版（Stable）”的语言状态对于限定在编译器到 TISC 这一工具链中是说得通的，但这未必适用于暗示在层级、代理和受控外部接口上拥有完整生态语意的全景，除非能提出更窄泛的发布界定。 ([GitHub][5])

作为一个稳定的运算基础，数据类型 + TISC + 非 JIT 模式的 T81VM 是该仓库最为可靠的技术资产。这一部分有可能被视为一套严谨的实验性平台。 ([GitHub][9])

## 13. 研究与产品化评估

当前，代码库处于“实验平台”和“产品前期的技术基础”之间。它不再仅仅是一个研究对象，因为它拥有构建系统、CI、测试、发布准则文档、符合性参考、仪表板以及明确划定的确定性核心。但是，它在严格意义上尚未成为开发者平台或基础设施的备选方案，因为过多的外围层仍然处于实验阶段、存在矛盾，或者相较于其实施的成熟度而言，承载了过重的治理结构。 ([GitHub][3])

若要迈入下一阶段，代码库亟需完成一次决定性的转变：通过更严苛的发布标识，将可认证的确定性核心从探索性的生态系统中分离出来。这意味着必须有一个权威的版本来源、单一的状态来源、统一的推广接口注册表，并在所有其他非核心部分明确标注“不属于产品边界”。在此之后，还需要在核心维护者团队之外实现独立的复现，并为它已宣称稳定的语言、工具链及治理接口提供更严密的证明。 ([GitHub][9])

## 14. SWOT（优势、劣势、机会、威胁）分析

**优势（Strengths）**
该代码库拥有一种真实的系统架构，而非仅限于品牌包装。其确定性的边界界定异常明晰。CI 与治理的规范也相当先进。围绕解释器构建的执行核心实质上是确切存在的。此外，该项目展现出扎实的文档记录习惯及跨层次设计的意图。 ([GitHub][2])

**劣势（Weaknesses）**
状态与版本间的不协调问题颇为严重。文档呈现出过度扩展的态势。有时候，关于治理的语言描述走在了实际实施的前面。操作系统以及高级的人工智能/治理层面功能的拓展速度，超越了可供审计的确定性的增加速度。命名上的重叠，尤其是在 Axion 相关的部分，使得架构变得模糊不清。 ([GitHub][13])

**机会（Opportunities）**
明确界定发布边界的确定性核心，能够成为该项目深入计算研究、可复现执行乃至语言/虚拟机实验等领域的坚实切入点。如果代码库不再试图将每个外围环节都表现得与核心同样成熟，它已经具备了支持这一愿景的治理框架。 ([GitHub][9])

**威胁（Threats）**
最大的威胁源于夸大其词导致的项目信誉受损。在一个以确定性和可审计性为根基的项目里，存在相互矛盾的状态展示极具破坏力。第二大威胁在于涉猎过广引发的维护体系崩溃：内核、语言、虚拟机、存储、人工智能、硬件、文档、书籍、仪表板以及多语种的对外沟通，这一切都被置于一个不断变动的权限体系之下。 ([GitHub][1])

## 15. 风险登记簿 (Risk Register)

| 风险项 | 受影响的系统 | 严重程度 | 证据 | 缓解措施 |
| ---------------------------- | ------------------------------------------------------------------------------ | -------: | ----------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| 状态/版本漂移 | 整个代码库 | 严重 | README 称 `v1.9.0` / 369 个测试，但控制中心称 `v1.4.1` / 363 个测试，而 CMake 称 `1.3.6` | 为版本、发布状态和测试总数建立唯一的事实来源；从中生成下游文档。 ([GitHub][1]) |
| 规范与实现不匹配 | ISA，VM，Axion | 严重 | TISC 规范 1.1 与 README 中的 TISC 1.2；VM 规范 Beta 版与仪表板中的 Stable 版；Axion 规范 Alpha 版与矩阵中的 Stable 版 | 增加自动化状态同步检查，核对规范版本/状态、仪表板和 README。 ([GitHub][12]) |
| 治理形式主义风险 | Axion，认知分层 | 高 | Axion 规范表示确定性管理是部分的；复杂性指标是部分的 | 将公开声明严格限制在已实现且经验证的实施点和钩子上。 ([GitHub][6]) |
| 过度扩展 | 实验性，AI，OS，硬件 | 高 | DCP 剔除了众多外部表面，而 README/路线图涵盖了硬件、确定性云、认知 | 冻结针对外部表面的营销；在根目录文档中发布一份“核心 vs 研究”的对照图。 ([GitHub][9]) |
| 术语/命名混乱 | Axion，TernaryOS，内核 | 高 | 进度日志称使用内部 `ternaryos` 的 Axion OS；规范将 Axion Kernel 作为监督层 | 持续重命名治理内核或操作系统内核，或者为它们添加前缀。 ([GitHub][7]) |
| 实验性内容扩散 | `experimental`, `experiments/ ai`, `legacy`, `internal`, notebooks/pdf/archive | 高 | 庞大的代码库表面，包含了大量支持状态不明的目录 | 按照顶层目录发布支持状态分类：核心维护、支持维护、实验性、遗留、归档、仅内部使用。 ([GitHub][1]) |
| 测试盲区 | VM 调度，编译器重现，JIT | 中 | 调度跟踪事件不是一级事件；编译器生成部分；JIT 未包含在 DCP 中 | 在进一步扩展功能之前，将缺失的跟踪面提升为一级测试。 ([GitHub][8]) |
| 维护风险 | 文档 + 代码 + 仪表板 | 中 | 庞大的文档网络，包含多个状态仪表板和权力层级 | 从机器可读的元数据中生成矩阵/仪表板。 ([GitHub][2]) |
| 入门风险 | 新贡献者 | 中 | 庞大的根目录树以及相互竞争的文档/规范/书籍/控制中心层级 | 添加贡献者路径图：“先信任哪里，暂时不信任哪里”。 ([GitHub][1]) |
| 信誉风险 | 外部合作伙伴/资助者 | 严重 | 内部不一致性损害了确定性/可审计的品牌形象 | 将同步缺陷视为阻碍发布的缺陷。 ([GitHub][1]) |

## 16. 成熟度计分卡

这些分数是我综合审查了相关的规范、面板、CI、DCP/注册表以及操作系统进度文档后得出的结论。这些是评估性质的判断，并非代码库本身提供的量化数据。 ([GitHub][2])

| 领域 | 概念清晰度 | 实现深度 | 测试证据 | 接口稳定性 | 治理清晰度 | 运行就绪度 | 文档完整性 |
| ---------------------------- | -----------------: | -------------------: | ------------: | ------------------: | -----------------: | --------------------: | ----------------------: |
| 数据类型 | 5 | 4 | 4 | 5 | 4 | 4 | 4 |
| TISC ISA | 5 | 4 | 4 | 5 | 4 | 4 | 3 |
| T81VM | 4 | 4 | 4 | 3 | 4 | 4 | 3 |
| T81Lang | 4 | 3 | 3 | 3 | 4 | 3 | 4 |
| Axion 治理 | 4 | 3 | 3 | 3 | 4 | 3 | 3 |
| CanonFS | 4 | 3 | 3 | 3 | 3 | 3 | 3 |
| CI / 治理工具 | 4 | 4 | 4 | 4 | 5 | 4 | 4 |
| Axion OS / TernaryOS | 4 | 3 | 4 | 2 | 3 | 2 | 3 |
| AI / 认知 / 分布式 | 3 | 2 | 2 | 2 | 3 | 1 | 3 |
| 文档 / 公共沟通 | 4 | 4 | 3 | 2 | 4 | 3 | 2 |

综合加权成熟度情况：**3.4 / 5**。这反映出一个严谨的实验性平台，具备可靠的确定性核心，但在外围层面、治理及文档方面存在的模糊性与偏移，阻碍了其获得更高的“基础设施就绪（infrastructure-ready）”评级。 ([GitHub][9])

## 17. 战略性建议

### 近期优先事项 (0–30 天)

统一版本/状态的权限。选择一个规范的信息源，用于发布版本号、规范成熟度、子系统成熟度和测试总数；从此信息源生成 README、控制中心 (Control Center)、实施矩阵 (Implementation Matrix) 及构建元数据。在完成此工作之前，每项关于“稳定 (stable)”的声明都应视为暂定的。 ([GitHub][1])

重命名两个 Axion 概念。一个是治理内核/策略引擎；另一个是实验性操作系统。它们需要不同的面向用户的名称或严格的前缀。 ([GitHub][6])

按顶层目录发布代码库支持状态索引：受维护核心、受维护支持、实验性、遗留、存档、仅限内部。目录树太大，不能含糊其辞。 ([GitHub][1])

### 短期优先事项 (1–3 个月)

严格遵守规范到实施的升级规则。如果规范仍处于 Beta/Alpha 阶段，则不应在面板上将子系统标记为稳定版 (Stable)，除非代码库明确区分“实施稳定，规范待定”。目前处理得不够清晰。 ([GitHub][8])

完善缺失的虚拟机/Axion 边界的监控面：直接的执行模式查询、一等的调度跟踪事件，以及遗留的规范原因字符串 (canonical reason-string) 和不确定性检测工作。这些都是关键的着力点，因为它们将治理从哲学层面转化为了具体的工具手段。 ([GitHub][8])

将 DCP 的发布说明与生态系统的发布说明分开。核心部分理应遵循一套精简、可验证的发布规范。而更为广泛的生态系统则应有一份研究进度记录。将二者混杂，会削弱双方的效果。 ([GitHub][9])

### 中期优先事项 (3–12 个月)

实现核心的独立外部复现，因为代码库本身将其视为阶段 2 验证剩余的进展标准。这比增加新的面板层对提升可信度的作用要大得多。 ([GitHub][1])

对于操作系统的开发，明确下一个试金石：是“基于严密设备/启动仿真的托管原型”，还是“受限目标环境下的真实内核底座”。当前其进展虽显负责，但战线铺得过长。制定更集中的验收目标，能让项目聚焦。 ([GitHub][7])

至于 AI 与治理的外围领域，停止将认知、分布式或硬件构想作为比肩核心的组件来推广。将这些内容明确列入非 DCP 的研究通道中，直到它们积累了与虚拟机、ISA 或数据类型技术栈同等强度的确凿证据。 ([GitHub][9])

## 18. 最终判定

在当前的状况下，T81 Foundation 最可信的身份是**一个严格实验性质的确定性计算平台。它不仅具有真实的 VM/ISA/数据模型核心，还配备了一套异常复杂的治理与文档上层建筑**。 ([GitHub][2])

它的核心技术优势在于**一个边界明确的确定性核心配置（DCP），以及代码库确实在试图将各种论断同有名称的测试、持续集成（CI）检查及明确声明的作用域排除范围相挂钩**。 ([GitHub][9])

而其最大的结构性隐患是**跨规范、仪表盘、自述文件（README）以及构建元数据之间存在的状态不一致**。对于一个普通的开源库而言，这或许只是令人不悦；但对于一个以确定性和可审计性为命脉的项目来说，这却是致命的短板。 ([GitHub][1])

如果要说有哪一项单一的调整能最有效地改善其发展轨迹，那就是**将“同步”这一过程本身作为一种一等公民的确定性边界来对待**：建立单一的权威依赖图谱，生成统一的成熟度总账，确立唯一的发布基准，并在可认证的核心区域与探索性的前沿研究之间划定一道不可逾越的鸿沟。

[1]: https://github.com/t81dev/t81-foundation/ "GitHub - t81dev/t81-foundation: T81 是一个统一的、确定性的、原生三态的计算架构，旨在突破二进制计算的限制。 · GitHub"
[2]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/architecture/OVERVIEW.md "raw.githubusercontent.com"
[3]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/.github/workflows/ci.yml "raw.githubusercontent.com"
[4]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/status/IMPLEMENTATION_MATRIX.md "raw.githubusercontent.com"
[5]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81lang-spec.md "raw.githubusercontent.com"
[6]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/axion-kernel.md "raw.githubusercontent.com"
[7]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/experimental/ternaryos/docs/PROGRESS.md "raw.githubusercontent.com"
[8]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81vm-spec.md "raw.githubusercontent.com"
[9]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/product/DETERMINISTIC_CORE_PROFILE.md "raw.githubusercontent.com"
[10]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/CMakeLists.txt "raw.githubusercontent.com"
[11]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/governance/DETERMINISM_SURFACE_REGISTRY.md "raw.githubusercontent.com"
[12]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/tisc-spec.md "raw.githubusercontent.com"
[13]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/status/PROJECT_CONTROL_CENTER.md "raw.githubusercontent.com"

---

## 许可证

MIT License。
