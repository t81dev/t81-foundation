# TISC — Ternary Instruction Set Computer
Version 0.1 — Draft

TISC defines the low-level execution model for T81 computations.

---

## 1. Machine Model
STATE = (R, PC, SP, FLAGS, MEM, META)

## 2. Register File
- 27 general-purpose registers  
- R23: accumulator  
- R24: condition register  
- R26: Axion system register  

## 3. Memory Model
- Base-81 aligned blocks  
- Code, stack, tensor heap, metadata segments  

## 4. Instruction Encoding
- 81 trits total  
- opcode + flags + operands + immediate  

## 5. Opcode Classes
- Arithmetic  
- Ternary logic  
- Comparison  
- Memory ops  
- Flow control  
- Tensor/matrix ops  
- Axion-privileged ops  

## 6. Fault Semantics
All faults MUST:

- be deterministic  
- be reproducible  
- be recorded by Axion  
- halt or trap cleanly  

