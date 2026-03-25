<p align="center">
  <img src="assets/banner.png" alt="T81 — 三元计算架构" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 — 三元计算架构

![Release](https://img.shields.io/badge/release-v1.9.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-Apache_2.0-blue)

利用基于 e 的计算的理论效率，**T81** 是一种基于 **平衡三元算术** ({-1, 0, +1}) 构建的确定性计算架构，拥有涵盖指令集、虚拟机、语言编译器和 AI 推理环境的全链条治理模型。

该架构提供：

- **验证表面上的比特级精确可重复性** — 受治理的确定性表面在受支持的平台上产生完全相同的跟踪哈希
- **受治理的 AI 推理** — Axion 策略引擎在任何副作用发生之前拦截并审计每一项特权操作
- **内容寻址来源** — CanonFS 以不可变方式记录所有工件、模型权重和运行时状态
- **针对受治理的纪元语义的确定性并行执行** — DPE 任务图模型 (RFC-DPE-0002) 使得在当前确定性表面下的并发 TISC 工作负载具备纪元提交的输出能力

---

## 目录
- [项目状态](#项目状态)
- [架构与生态系统概览](#架构与生态系统概览)
- [关键组件](#关键组件)
- [快速入门](#快速入门)
- [确定性验证](#确定性验证)
- [文档](#文档)
- [治理](#治理)
- [三元优势](#三元优势)
- [许可证](#许可证)

---

## 项目状态

**阶段：积极开发** — v1.9.0-Stable；369/369 测试通过；已在 Linux x86\_64 + macOS ARM64 上验证跨平台确定性。

确定性表面分类遵循确定性表面注册表和 RFC-0048 引入的治理模型：

- **DCP / 经过验证的确定性表面**：语义、边界和重放证明受治理并由 CI 强制执行
- **受治理的非 DCP**：受策略约束且在架构上很重要，但尚无资格提出完整的确定性声明
- **实验性**：活跃的设计或验证表面；不提出确定性保证声明

| 组件 | 成熟度 | 备注 |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.9.0；操作码语义在 v1.x 下不可变；`AgentInvoke` (RFC-0015)，6 个三元原生推理 (RFC-0034)，3 个 FFI (RFC-00B8)，2 个格密码 (RFC-0038)，1 个 KEM 环 (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt，Float，Complex，Map，Set — 位稳定编码；审计干净 |
| **T81VM** | ✅ Stable | 解释器的 DCP / 已验证的确定性表面以及当前支持平台的跟踪奇偶校验；具有 `AgentInvoke`、三元原生推理、FFI、格密码和 NTRU-KEM 操作码的完整 TISC v1.9.0 分派；369/369 测试 |
| **T81Lang** | ✅ Stable | 整体受治理的非 DCP：具有主动编译器确定性控制的 v1.9.0 Stable 规范，但编译器发射仍部分经过验证，而不是完全升级为经过验证的确定性表面 |
| **Axion Governance Kernel** | ✅ Stable | 整体受治理的非 DCP：规范的推理字符串和审计钩子处于活动状态，但完整的内核/治理表面比当前验证的确定性注册表更广泛 |
| **Ternary-Native Inference** | ✅ Stable | 受治理的非 DCP：RFC-0034 + RFC-0037 操作码/运行时/stdlib 表面已实现并提供证据，但并非所有与推理相邻的执行路径都已升级为经过验证的确定性表面 |
| **Lattice Cryptography** | ✅ Stable | 受治理的非 DCP：RFC-0038+0039 表面已实现并受策略约束；确定性升级仍然是特定于表面的，而不是隐含在整个加密垂直领域 |
| **Governed FFI** | ✅ Stable | 受治理的非 DCP：RFC-00B8 + RFC-0036 VM/语言桥已端到端实现，但在进行更强有力的确定性声明之前，沙箱和更广泛的模式升级仍然开放 |
| **TUI Frontends** | ✅ Beta | 受治理的非 DCP：操作员和代理 TUI 是生产可用的界面，但 UI/运行时集成身并不是经过验证的确定性表面 |
| **DPE (Parallel Execution)** | ✅ Stable | 受治理的确定性执行模型与接受的 RFC-DPE-0001–0009；确定性纪元语义就绪，而更广泛的表面升级仍然受注册表和配套 RFC 链的治理 |
| **Cognitive Tiers** | ✅ Beta | 实验性 / 非 DCP：Tier4 认知仍然受治理约束，但不是经过验证的确定性表面 |
| **TernaryOS User Environment** | ✅ Beta | 受治理的非 DCP / beta：已实现并受策略约束，但目前未作为经过验证的确定性表面呈现 |
| **Axion OS** | ✅ Alpha | 受治理的非 DCP / alpha：主动治理架构，但尚未作为整体进行完全推广验证的确定性表面 |

---

## 架构与生态系统概览

```text
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
│  TISC ISA v1.9.0  ❄️ Frozen  +  Data Types  ❄️ Frozen       │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  FFIDispatcher · FFILibraryRegistry                         │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS · Cognitive Tiers
```

### 关键组件

**TISC ISA v1.9.0** — 三元指令集架构。在 v1.x 下冻结；整个堆栈的不可变执行合约。

**T81VM** — 确定性 TISC 解释器。保证跨平台输出位相同；Axion 的分派前隔离将治理钩子保留在热执行路径之外。

**Axion Governance Kernel** — 策略引擎，在产生任何副作用之前拦截 `AXREAD`、`AXSET`、`AXVERIFY`、AI 操作码和 FFI 调用。如果策略解析失败，则默认拒绝 (Fail-closed)。

**CanonFS** — 内容寻址文件系统。将所有代码对象、模型权重和运行时工件存储为不可变的、哈希标识的 blob。为确定性审计提供来源证明。

**T81Lang** — 目标为 TISC 字节码的高级语言。原生类型：`BigInt`、`Fraction`、`Float`、`Complex`、`Tensor`、`Map`、`Set`。编译器流水线：词法分析器 → 解析器 → 类型化 AST → 语义分析 → IR 生成。

**Ternary-Native Inference (RFC-0034)** — 六个 TISC 操作码，用于使用平衡三元权重 {−1, 0, +1} 的无乘法 AI 推理：`TWMATMUL`（矩阵乘法）、`TQUANT`（量化为 trit）、`TATTN`（三元注意力）、`TWEMBED`（权重嵌入）、`TERNACCUM`（标量点积）、`TACT`（带 Axion 封顶门限的激活）。T81WTN 权重格式。通过 RFC-0036 完成了 T81Lang `foreign {}` 前端。

**Governed FFI (RFC-00B8 + RFC-0036)** — 全栈受治理的外部函数接口。VM 层（RFC-00B8 阶段 1）：`FFIDispatcher` 在任何外部调用前执行策略检查、资源配额和审计跟踪；`FFILibraryRegistry` 按名称和版本哈希跟踪已注册的库；三个 VM 操作码（`FFICall`、`FFIRegister`、`FFIPolicySet`）。语言层（RFC-0036）：`foreign deterministic { fn sin(x: T81Float) -> T81Float; }` 声明签名；在调用站点处 `foreign.sin(angle)` 降级为 `FFI_CALL`，函数名包含在 `text_literal` 中。

**TUI Frontends** — 两个互补的终端界面，构建在 FTXUI v5.0.0 之上：
- `t81 studio` — 导航侧边栏、CanonFS 浏览器、Axion 仪表板、确定性跟踪可视化器、命令面板（`Ctrl+P`）
- `t81 agent` — 持久化 JSONL 会话，斜杠命令（`/compile`、`/run`、`/hash`、`/allow`、`/infer`、`/trits` 等），trit 概率条

**DPE (Deterministic Parallel Execution)** — 在冻结的 TISC ISA 之上的任务图模型。任务声明不可变输入和缓冲的输出区域；VM 在纪元结束时原子地提交所有写入。无需新的操作码。

---

## 快速入门

### 从源码构建

```bash
# Clone the repository
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# Configure and build (Release mode)
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the test suite (369 tests)
ctest --test-dir build --output-on-failure
```

### CLI 与接口

```bash
# Compile a T81Lang program
./build/t81 code build examples/hello.t81 -o hello.tisc

# Execute with Axion governance
./build/t81 vm run hello.tisc

# Launch the human operator TUI
./build/t81 studio

# Launch the AI-native TUI
./build/t81 agent
```

---

## 确定性验证

对经过验证的确定性表面检查以确保比特级精确的跨平台重现性。

```bash
./scripts/ci/run_determinism_slice.sh
```

当前核心表面的已验证平台：**Linux x86_64**、**macOS ARM64**。在已验证的确定性表面上的任何差异都属于严重缺陷。

---

## 文档

| 主题 | 位置 |
| :--- | :--- |
| 入门 (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| 入门 (AI) | `docs/user-guide/getting-started/ai-quickstart.md` |
| TUI 指南 | `docs/user-guide/how-to/tui-guide.md` |
| ISA 规范 | `spec/tisc-spec.md` |
| Axion 策略手册 | `docs/user-guide/tutorials/axion-policy-manual.md` |
| T81Lang Stdlib 参考 | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| 架构概览 | `docs/architecture/OVERVIEW.md` |
| 治理章程 | `docs/governance/README.md` |
| 项目控制中心 | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## 治理

T81 Foundation 在 **持续治理 (C2)** 模式下运作。所有贡献必须保持：

- **确定性执行平价** — 跟踪哈希在支持的平台上必须匹配
- **架构一致性** — 触及确定性表面的更改需要正式审查
- **再现性保证** — DCP 表面中不存在浮点或平台特定的非确定性

确定性表面在 `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` 中定义。对冻结表面（TISC ISA、Data Types）的更改需要大版本升级。

> **边界注意：** DCP、受治理的非 DCP、实验性和超出范围的分类在 RFC-0048 中宪法式地定义。公开文档不得将受治理的非 DCP 或实验性表面显示为经过验证的确定性组件。

---

## 三元优势

尽管现代二进制硬件已针对通用计算进行了高度优化，但 **T81 Foundation** 利用了 **平衡三元** ({−1, 0, +1}) 独特的数学与结构属性，实现了传统二进制系统难以或无法企及的优势 —— 尤其在确定性执行、受控 AI 推理以及低复杂度神经网络负载方面。

### 1. O(1) 计算对称性 — 无进位取反

在二进制的二进制补码中，取反需要逐位 NOT 运算再加 1，这可能会触发长进位链。在平衡三元制中，取反只需翻转每个非零 trit 的符号 (+1 ↔ −1, 0 保持 0) — **零进位传播**，恒定时间。

- **实测性能**：PackedCell 取反在近期的 x86_64 硬件上达到 **~49.9 G-ops/s**，优于经过优化的 64 位整数取反 **~10.9×**（基准测试已在 Linux x86_64 和 macOS ARM64 上验证）。
- 这种对称性自然地扩展到许多算术模式，减少了符号繁重的操作的延迟和功耗。

### 2. 卓越的基数经济性与理论密度

对于位置数字系统而言，信息论的最优基数接近 *e ≈ 2.718*。三元制（基数 3）在数学上比二进制（基数 2）更接近，提供 **~1.585 bit 的信息量每 trit** (log₂(3))。

- 在实践中，这可以提高每位数字的熵和更紧凑的对称范围表示 — 对于神经网络权重、嵌入、坐标系和大规模稀疏张量特别有价值。

### 3. 固有的比特级精确确定性与独立于平台的舍入

IEEE 754 浮点数存在破坏可重复性的平台特定舍入模式、结合律差异和次正常处理问题。平衡三元算术本质上是对称于零的：

- 舍入采用简单的截断 — 无依赖方向的偏差。
- 每条执行路径在受支持的平台上产生 **完全相同的跟踪哈希**（目前经过验证的 Linux x86_64 + macOS ARM64 达到 100% 校验通过）。
- 这为科学计算、AI 推理审计和受控代理运行提供了坚如磐石的再现性。

### 4. 无乘法的神经推理

平衡三元权重 {−1, 0, +1} 允许进行 **无乘法的点积** — 将 MUL 替换为有条件的 ADD/SUB（或者在跳过零时进行纯粹的累加）。结合自定义 TISC 操作码（`TWMATMUL`、`TQUANT`、`TATTN`、`TWEMBED`、`TERNACCUM`、`TACT`）：

- 可以为 AI 推理提供极低的功耗和更高的吞吐量。
- 契合了 2024-2026 年在极低比特模型（BitNet b1.58, xTern, 三元 Transformer）方面的发展趋势，在精度损失极小的前提下，相比 FP16/FP32 基准能够实现 15-60× 的能量缩减及 4-90× 的吞吐量增长。
- T81WTN 权重格式 + 三元原生运算使得这项优势在该技术栈中达到了生产就绪级别。

### 5. 架构级治理与安全钩子

由于整个 TISC ISA 是原生三元的，**Axion 治理内核** 能够在任何副作用发生前，在 **trit 级别的细粒度** 下拦截并审计状态转换。这允许：

- 对特权操作（AI 调用、FFI 调用、代理行为）实施默认拒绝（Fail-closed）策略执行。
- 细粒度的道德门限、通过 CanonFS 进行来源跟踪，以及确定性的审计跟踪。
- 一个从根本上比黑盒二进制执行更具可检查性的安全模型。

这些优势在那些最看重 **再现性**、**低复杂度推理**、**受控执行** 和 **数学对称性** 的领域中产生了叠加效应 — 这正是 T81 架构旨在解决的核心应用场景。
所有确定性声明必须受到确定性表面注册表的严格限制。

---

## 许可证

Apache 许可证 2.0。
