# 第 9 章：Axion 安全内核

## 9.1 形式化定义

**状态：已实现并测试**

**Axion 内核** 是 T81 运行时的守护者。与在系统调用边界（用户/内核模式切换）强制执行安全性的传统操作系统不同，Axion 在 **指令级别** 强制执行安全性。

形式上，Axion 内核是一个函数 $\alpha$，它将当前机器状态 $S$ 和建议的操作 $Op$ 映射到一个裁决 $V$：
$$
\alpha: (S, Op) \to \{\text{Allow}, \text{Deny}, \text{Warn}, \text{Defer}\}
$$

此评估发生在状态转换 $S \xrightarrow{Op} S'$ 发生 **之前**。如果 $\alpha(S, Op) = \text{Deny}$，转换被中止，机器陷入 `SecurityFault`。

## 9.2 策略模型

**状态：已实现**

策略是声明性的规则集，定义了特定执行上下文的约束。策略不规定 *计算什么*，而是规定 *如何* 允许计算。

### 9.2.1 策略语言 (S-表达式)
Axion 策略使用类似 Lisp 的 S-表达式语法定义，确保易于解析和规范化。

**示例：严格的第 1 层策略**
```lisp
(policy
  (tier 1)                  ; 限制为符号层（无递归，无反射）
  (max-instructions 10000)  ; 硬 Gas 限制
  (max-stack 256)           ; 栈深度限制
  (max-tensors 0)           ; 不允许分配张量
  (allowed-tensor-hashes []) ; 不允许外部权重
)
```

**示例：第 3 层 AI 推理策略**
```lisp
(policy
  (tier 3)
  (max-recursion 1024)
  (max-tensors 50)
  (max-tensor-elements 1000000)
  (allowed-tensor-hashes [
    "canon:sha3:a7f..." ; 特定的允许模型权重
  ])
)
```

### 9.2.2 能力 (Capabilities)
能力是授予进程的细粒度权限。
*   **NetAccess**：使用 `IoNet` 句柄的能力（第 4 层）。
*   **MetaWrite**：修改元数据段的能力（反射）。
*   **InfExpand**：实例化无限形式的能力（第 5 层）。

## 9.3 指令拦截

**状态：已实现并测试**

Axion 内核直接集成到 VM 的取指-译码-执行循环中。

### 9.3.1 拦截器钩子
在 `src/vm/vm.cpp` 中，主循环调用策略引擎：

```cpp
// 解释器循环伪代码
while (!halted) {
    Opcode op = fetch();

    // 1. Axion 检查
    Verdict v = axion->evaluate(ctx);
    if (v == Verdict::Deny) {
        throw SecurityFault(v.reason);
    }

    // 2. 执行
    execute(op);

    // 3. 审计日志
    if (v == Verdict::Warn || policy.audit_all) {
        trace.log(op, v, state_hash);
    }
}
```

### 9.3.2 零成本抽象？
不。如果零成本抽象损害安全性，T81 明确拒绝它们。Axion 检查会带来性能开销。这是一个故意的设计选择：**正确性 > 性能**。然而，对于 JIT 编译的追踪，策略检查在追踪记录期间执行一次，并作为受保护的断言烘焙到优化的追踪中，从而显着减少运行时开销。

## 9.4 审计日志 (追踪)

**状态：已实现并测试**

**追踪 (Trace)** 是发生了什么的加密证明。它不仅仅是一个调试日志；它是事件的 Merkle 链。

### 9.4.1 追踪结构
日志中的每个条目包含：
1.  **Tick**：逻辑时钟时间。
2.  **Opcode**：执行的指令。
3.  **Verdict**：Axion 的决定。
4.  **StateHash**：操作 *之后* 相关机器状态的 SHA3-256 哈希。

$$
H_{t} = \text{Hash}(H_{t-1} || \text{Op}_t || \text{Verdict}_t || \text{StateDiff}_t)
$$

最终哈希 $H_n$ 是 **执行证明**。如果两方运行相同的代码并获得相同的 $H_n$，他们在密码学上保证通过完全相同的路径达到了完全相同的状态。

## 9.5 认知提升

**状态：已实现**

程序从特定的认知层（通常是第 1 层）开始。它可能会请求 **提升 (Promotion)** 到更高的层级以执行更复杂的操作。

*   **请求**：程序执行带有签名能力令牌的 `Promote` 操作码。
*   **评估**：Axion 根据策略验证令牌。
*   **结果**：如果允许，VM 的 `tier_status` 更新，解锁新的操作码（例如，`Recurse` 或 `Gossip`）。

**层级升级路径**：
1.  **第 1 层**：安全、有界、多项式时间。
2.  **第 2 层**：动态、反射。
3.  **第 3 层**：递归、指数时间潜力（需要 Gas 限制）。
4.  **第 4 层**：非本地、网络依赖（需要共识限制）。
5.  **第 5 层**：无限（需要严格遏制）。

> **验证**：`tests/cpp/test_ethics.cpp` 验证了在第 1 层策略中尝试使用第 3 层操作码会导致 `SecurityFault`。

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
