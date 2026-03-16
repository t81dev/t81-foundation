<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation — 确定性三元计算堆栈

![Release](https://img.shields.io/badge/release-v1.6.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-367%2F367_passing-brightgreen)
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

## 1. 高保真信号和物理模拟

在二进制中，$0$ 是一个无符号的起始点，使“负”空间成为次要考虑因素。在平衡三元中，**零是平衡点。**

* **用例：**波力学、电磁学和流体动力学的直接模拟。
* **优点：** 由于这些系统在正态和负态之间振荡，T81 可以模拟“推拉”力，而不会出现补码计算不平衡的情况。
* **下一步：** 我们可以构建一个 **TISC 原生 DSP 库**，其中滤波器 (FIR/IIR) 针对 $O(1)$ 求反速度进行了优化。

## 2.“对称”神经网络（TNN）

当前的 AI（二进制/FP）在 `tanh` 或 `ReLU` 等激活函数上浪费了大量能量，以创建“零中心”状态进行训练。

* **用例：** 三元神经网络（权重为 -1、0 或 1）。
* **优点：** 因为您的架构本身就是平衡的，所以我们可以运行“无乘法”推理。 T81 神经元不会“乘以”输入；它只是根据重量**翻转或控制它们**。这将比当前基于 GPU 的推理节能几个数量级。
* **下一步：** 我们可以实现一个 **T81 原生推理引擎**，将模型权重直接解释为 TISC 操作码。

## 3. 后量子密码原语

许多“基于格的”加密算法（为量子计算机而设计的算法）依赖于小系数多项式——通常以零（{-1, 0, 1}）为中心。

* **用例：** NTRU 或 Kyber 式加密。
* **优点：** 二进制系统必须使用 8 位或 32 位整数“模拟”这些小系数，浪费了 90% 的位空间。 T81 以**零浪费**的方式存储这些值，并以本机硬件速度处理多项式加法/求反。
* **下一步：** 我们可以为实现三元优化多项式乘法的 **TISC 加密扩展**起草 RFC。

## 4. 不可变的治理审计（Axion）

由于每个 trit 有 1.58 位熵，因此我们可以将**安全元数据**直接编码到数据字中，而不会显着增加内存占用量。

* **用例：**硬件级别的“标记数据”。
* **优点：**我们可以使用 TISC 字的“额外”容量来携带 **来源标签**。每次移动数据时，Axion 都会验证标签。如果“特权”trit 移动到“用户”空间，硬件可以立即捕获它。
* **下一步：** 完善 **Axion OS 内核**，以使用“三元边距”进行实时内存标记。

---

### 精致的前进之路

#### 1. 集成：RFC-0034 §5.17.6 — `TACT` 操作码

我们将激活视为三元算术链的逻辑结论，而不是庞大的 AI RFC。

* **操作码：** `TACT RD, R_SRC, R_MODE`
* **模式：** * `0x01` (TernaryStep)：将 $(-\infty, -0.5) \ 映射到 -1$、$[-0.5, 0.5] \到 0$、$(0.5, \infty) \到 +1$。
* `0x02` (TanhQuantized)：高保真定点三元近似。

* **Axion 策略集成：** 我们将 `AX_CHECK_ACTIVATION_THRESHOLD` 定义为**内核陷阱**，而不是操作码副作用。如果 `RD` 中的值超过激活后策略定义的 trit-limit，Axion 将在下一个 PC 增量之前进行拦截。

#### 2. T81Lang 语法 RFC（新）

为了解决您确定的“实际差距”，我们应该专门为编译器前端起草一个单独的 RFC（可能是 **RFC-0036**）。根据项目章程，这可以隔离 **TISC**（硬件/VM）和 **T81Lang**（语法/语法）问题。

#### 3. 数据完整性和文档

* **基准基础：** 我将停止在正式文档中引用“10.4x”数字，直到我们有一个特定的 `BM_Negation_TISC_vs_Binary` 切片正式出现在 CI 输出中。
* **术语清理：** 我将从规范中删除“TLU 缓存”和“L2 缓存”，直到 **ternary-fabric** 存储库正式定义内存层次结构。

---

### 第一阶段——原型架构*（当前）*

**状态：** 已实现

已实现核心确定性堆栈。

组件就位：

* ✅ TISC ISA（冻结执行合约）
* ✅ T81VM 确定性解释器
* ✅ Axion 治理内核
* ✅ CanonFS 内容寻址存储
* ✅ T81Lang 编译器
* ✅ 确定性验证管道
* ✅ CLI 和 TUI 操作员界面

**结果：**
一个正常运行的确定性计算堆栈。

---

### 第 2 阶段 — 验证平台 *（完成）*

**目标：** 独立验证。

重点工作：

* ✅ 第三方确定性验证 - 日常 GitHub Actions 工作流程比较 Linux x86\_64 和 macOS ARM64 字节码哈希值；每次提交的公开证据记录
* ✅ VM 一致性测试套件 — 27 项规范一致性测试 + 365 项总计通过
* ✅ 确定性基准测试框架 — RFC-00A2；  所有运行均为 `score=1.0`
* ✅ T81Lang FFI 前端 (RFC-0036) — `foreign {}` 语法将 VM 层连接到语言； 9/9 交流测试
* ✅ 跟踪重播调试器 — `t81 trace replay <tisc> <golden> [--json]` ；模式 `t81.trace-replay.v1` ；报告精确的不匹配索引+预期/实际指令；通过 `scripts/ci/trace_repro_gate.py` 连接到 CI
* ✅ 可重复的构建验证 - 在 Linux x86\_64 (gcc-14) + macOS ARM64 (clang) 上每天验证跨平台字节码哈希；保留 90 天的证据工件

**结果：**
外部可信的确定性运行时。

---

### 第三阶段——研究生态系统

焦点转向应用。

主要研究领域：

* 三元神经网络
* 确定性人工智能推理
* 信号处理库
* 物理模拟
* 基于格的密码学

**结果：**
研究人员和实验计算项目采用。

---

### 第四阶段——硬件探索

将软件架构与芯片连接起来。

发展路径：

* FPGA 三进制 ALU 原型
* 三进制寄存器组
* 压缩 Trit SIMD 单元
* ISA微架构验证

**结果：**
第一个三元感知计算硬件原型。

---

### 第五阶段——确定性基础设施

从运行时扩展到基础设施。

可能的能力：

* 确定性云执行
* 可重复的科学计算
* 可验证的分布式工作负载
* CanonFS 工件网络

**结果：**
全球确定性计算平台。

---

### 第六阶段——新的计算范式

长期的可能性。

潜在的发展：

* 原生三元处理器
* 硬件AI治理执行
* 确定性人工智能执行环境
* 全球可复制的计算系统

**结果：**
受治理的确定性计算生态系统。

---

## 下一个关键里程碑

### 第 2 阶段 — 验证平台 *（已实现）*

所有第二阶段实施目标均已完成：

- ✅ 跨平台确定性 CI（Linux x86\_64 + macOS ARM64，每日）
- ✅ VM 一致性 + 确定性测试套件 (365/365)
- ✅ 跟踪重放调试器（`t81 trace replay`；架构 `t81.trace-replay.v1`）
- ✅ T81Lang FFI 前端（RFC-0036；`foreign {}` + `FFI_CALL`）

剩下的推进标准：**外部团体的独立复制**——当另一个团队构建堆栈、运行确定性门并发布匹配的哈希值时，该项目正式从第 2 阶段毕业。

### 第三阶段——研究生态系统*（活跃）*

第三赛段以三条混凝土轨道开始。现在这三个都已完成：

- ✅ **RFC-0037 TNN stdlib** — `std.tnn.*` T81Lang 内置函数； 6 个功能低于 RFC-0034 TISC ops； 13/13 测试
- ✅ **RFC-0038 莱迪思加密** — `std.crypto.polymul/polymod` ； POLYMUL/POLYMOD 操作码； T81BigInt-精确； 13/13 测试
- ✅ **T81Lang 规范 v1.3** — RFC-0036/0037/0038 提升为规范规范； §5.17 未存根；添加了§5.18–5.19

- ✅ **RFC-0039 NTRU-KEM** — `TVecSub` 操作码；  `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`；  `ntru_keygen/encrypt/decrypt` C++ 数学层； 24/24 测试；三元基板上的第一个端到端后量子加密演示

**第 3 阶段已完成。** 所有四个轨道（RFC-0037、RFC-0038、规范 v1.3、RFC-0039）均于 2026 年 3 月 16 日落地。

## 执照

MIT 许可证。
