# T81 Core Data Types

This directory contains the foundational data types for the T81 ecosystem. These header-only C++20 classes provide a complete suite of tools for balanced ternary computation, from primitive arithmetic to complex, higher-level constructs.

## Canonical Data Types

The following table provides a comprehensive inventory of all canonical data types defined in this module.

| Data Type | Header File | Description |
|---|---|---|
| **Core Primitives** | | |
| `T81Int<N>` | [T81Int.hpp](./T81Int.hpp) | Balanced ternary integer with packed trits. |
| `T81UInt<N>` | [T81Uint.hpp](./T81Uint.hpp) | Defines the T81UInt class for unsigned balanced-ternary integers. |
| `T81BigInt` | [T81BigInt.hpp](./T81BigInt.hpp) | High-level ternary integer wrapper for arbitrary-precision arithmetic. |
| `T81Float<M,E>` | [T81Float.hpp](./T81Float.hpp) | Balanced ternary floating-point backed by T81Int storage. |
| `T81Fixed<I,F>` | [T81Fixed.hpp](./T81Fixed.hpp) | Balanced ternary fixed-point arithmetic on top of T81Int. |
| `T81Fraction<N>` | [T81Fraction.hpp](./T81Fraction.hpp) | Exact rational arithmetic over balanced ternary integers. |
| `T81Complex<M>` | [T81Complex.hpp](./T81Complex.hpp) | Balanced ternary complex numbers backed by T81Float. |
| `T81Quaternion` | [T81Quaternion.hpp](./T81Quaternion.hpp) | Defines the T81Quaternion class for 3D/4D geometry and rotations. |
| `T81Prob` | [T81Prob.hpp](./T81Prob.hpp) | Defines the T81Prob class for native log-odds probability representation. |
| `T81Polynomial` | [T81Polynomial.hpp](./T81Polynomial.hpp) | Defines the T81Polynomial class for exact polynomial arithmetic. |
| | | |
| **Symbolic & Raw Data** | | |
| `T81Symbol` | [T81Symbol.hpp](./T81Symbol.hpp) | An eternal, unique, 81-trit identifier. |
| `T81String` | [T81String.hpp](./T81String.hpp) | Variable-length text for the T81 stack. |
| `T81Bytes` | [T81Bytes.hpp](./T81Bytes.hpp) | Lightweight byte buffer for canonical, deterministic byte handling. |
| `Base81String` | [base81.hpp](./base81.hpp) | Defines the Base81String type and validation utilities for Base-81 encoding. |
| | | |
| **Containers & Structures** | | |
| `T81List<E>` | [T81List.hpp](./T81List.hpp) | A dynamic, ternary-native list container. |
| `T81Map<K,V>` | [T81Map.hpp](./T81Map.hpp) | A ternary-optimized associative map. |
| `T81Set<T>` | [T81Set.hpp](./T81Set.hpp) | An immutable, ternary-native set. |
| `T81Tree<T>` | [T81Tree.hpp](./T81Tree.hpp) | An immutable ternary tree (left, middle, right) with shared structure. |
| `T81Stream<T>` | [T81Stream.hpp](./T81Stream.hpp) | Defines the T81Stream class for infinite, lazy sequences. |
| `T81Graph` | [T81Graph.hpp](./T81Graph.hpp) | Defines the T81Graph class, a static graph structure for high performance. |
| | | |
| **Numerical Containers** | | |
| `T81Vector<N,S>` | [T81Vector.hpp](./T81Vector.hpp) | A mathematical vector with physical semantics. |
| `T81Matrix<S,R,C>` | [T81Matrix.hpp](./T81Matrix.hpp) | A container for matrices of balanced-ternary, tryte-based scalar types. |
| `T81Tensor<E,R,Dims...>`| [T81Tensor.hpp](./T81Tensor.hpp) | A multi-dimensional array for high-performance numerical computing. |
| | | |
| **Semantic & Flow Control** | | |
| `Option<T>` | [Option.hpp](./Option.hpp) | Alias for the language-facing `Option` built on `T81Maybe`. |
| `Result<T,E>` | [Result.hpp](./Result.hpp) | Alias that maps the surface `Result` type to `T81Result`. |
| `T81Maybe<T>` | [T81Maybe.hpp](./T81Maybe.hpp) | A ternary-native optional / Maybe type for handling absence of a value. |
| `T81Result<T,E>` | [T81Result.hpp](./T81Result.hpp) | Represents a success (with a value) or a failure (with an error). |
| `T81Promise<T>` | [T81Promise.hpp](./T81Promise.hpp) | A container for a value that may be computed asynchronously. |
| | | |
| **Civilizational & Reflective Types** | | |
| `T81Agent` | [T81Agent.hpp](./T81Agent.hpp) | A self-contained cognitive entity with identity, beliefs, and memory. |
| `T81Entropy` | [T81Entropy.hpp](./T81Entropy.hpp) | Provenanced entropy tokens for thermodynamic accounting of operations. |
| `T81Time` | [T81Time.hpp](./T81Time.hpp) | A lightweight time utility for deterministic, monotonic timekeeping. |
| `T81IOStream` | [T81IOStream.hpp](./T81IOStream.hpp) | A stream for entropy-costing, reflective I/O operations. |
| `T81Thread` | [T81Thread.hpp](./T81Thread.hpp) | A mechanism for reflective, named concurrency. |
| `T81Network` | [T81Network.hpp](./T81Network.hpp) | A system for ternary-native, reflective networking. |
| `T81Discovery` | [T81Discovery.hpp](./T81Discovery.hpp) | A zero-configuration peer discovery protocol for T81 agents. |
| `T81Qutrit` | [T81Qutrit.hpp](./T81Qutrit.hpp) | A type for representing a ternary quantum state. |
| `T81Category` | [T81Category.hpp](./T81Category.hpp) | C++ templates for representing concepts from category theory. |
| `T81Proof` | [T81Proof.hpp](./T81Proof.hpp) | A class for representing formal, verifiable proofs. |
| `T81Reflection` | [T81Reflection.hpp](./T81Reflection.hpp) | A minimal reflection wrapper for values. |
| | | |
| **Low-Level Utilities** | | |
| `Cell` | [cell.hpp](./cell.hpp) | A 5-trit balanced ternary cell, the fundamental unit of storage. |
| `CanonicalId` | [ids.hpp](./ids.hpp) | A struct for representing canonical identifiers. |
| `all.hpp` | [all.hpp](./all.hpp) | A convenience header to include all core T81 data types. |
