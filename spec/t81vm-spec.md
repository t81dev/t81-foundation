# T81 Virtual Machine Specification
Version 0.1 — Draft

The T81VM provides deterministic execution for TISC programs.

---

## 1. Execution Modes
- Reference interpreter  
- Deterministic JIT  

## 2. Determinism Constraints
VM MUST produce identical output across all systems.

## 3. Concurrency Model
- Message-passing  
- Race-free  
- Deterministic scheduling  

## 4. Memory Model
- Deterministic GC  
- Explicit object graph  
- Identity-preserving mutations  

## 5. Axion Interface
- Trace generation  
- Safety hooks  
- Entropy modeling  
- Recursion limits  

