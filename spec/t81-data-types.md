# T81 Data Types Specification
Version 0.1 — Draft

The T81 Data Types define the core numerical and structural primitives of the ecosystem.

---

## 1. Design Goals

- Base-81 numeric semantics  
- Deterministic normalization  
- Zero undefined behavior  
- Axion-visible structure  
- Portable across all layers  

---

## 2. Primitive Types

### **T81BigInt**
- Arbitrary-precision base-81 integer  
- No leading zeroes  
- Canonical sign format  
- Supports all TISC arithmetic ops  

### **T81Float**
- Deterministic, reproducible floating-point format  
- Base-81 mantissa and exponent  
- Guaranteed round-trip conversions  

### **T81Fraction**
- Exact rational type  
- Stored as reduced numerator/denominator BigInts  

### **Trit**
Balanced ternary digit:  
`−1, 0, +1`

---

## 3. Composite Types

### **Vectors / Matrices / Tensors**
- Multidimensional arrays  
- Deterministic layout  
- Base-81 aligned memory rules  
- Valid operands for TISC tensor ops  

### **Graphs**
- Node/edge lists  
- Axion-verifiable metadata  

---

## 4. Structural Types

- Arrays  
- Records  
- Structs  
- Enums  

All structural types must have:

- deterministic iteration order  
- explicit memory layout  
- no implicit coercions  

---

## 5. Normalization Rules

- All values must have exactly one canonical form.  
- No NaN, no infinities, no undefined states.  
- T81Fraction must always remain reduced.  
