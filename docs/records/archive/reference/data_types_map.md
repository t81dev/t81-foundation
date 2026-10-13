# Data Types and Representation Map

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Data Types and Representation Map](#data-types-and-representation-map)
  - [Core Primitives](#core-primitives)
  - [Symbolic & Raw Data](#symbolic-&-raw-data)
  - [Containers & Structures](#containers-&-structures)
  - [Numerical Containers](#numerical-containers)
  - [Semantic & Flow Control](#semantic-&-flow-control)
  - [Civilizational & Reflective Types](#civilizational-&-reflective-types)
  - [Neural Network & AI](#neural-network-&-ai)
  - [Low-Level Utilities](#low-level-utilities)

<!-- T81-TOC:END -->


This document provides a comprehensive map of the T81 canonical data types, their headers, descriptions, and underlying C++ memory representations.

## Core Primitives

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `T81Int<N>` | `t81/core/T81Int.hpp` | Balanced ternary integer with packed trits. | `std::array<std::uint8_t, (N+3)/4>` (2 bits/trit, 4 trits/byte) |
| `T81UInt<N>` | `t81/core/T81Uint.hpp` | Unsigned balanced-ternary integer (positive half). | `T81Int<N>` wrapper (clamped to non-negative) |
| `T81BigInt` | `t81/core/T81BigInt.hpp` | Arbitrary-precision ternary integer. | `std::vector<Limb>` + `bool negative_` |
| `T81Limb` | `t81/core/T81Limb.hpp` | 48-trit (16-tryte) packed integer unit for BigInt. | `int8_t[16]` (packed trytes) |
| `T81Float<M,E>` | `t81/core/T81Float.hpp` | Balanced ternary floating-point. | `T81Int<1 + E + M>` (Sign + Exponent + Mantissa) |
| `T81Fixed<I,F>` | `t81/core/T81Fixed.hpp` | Fixed-point arithmetic. | `T81Int<I + F>` (scaled integer) |
| `T81Fraction<N>` | `t81/core/T81Fraction.hpp` | Exact rational number. | Pair of `T81Int<N>` (`num_`, `den_`) |
| `T81Complex<M>` | `t81/core/T81Complex.hpp` | Complex number. | Pair of `T81Float<M, 9>` (`re`, `im`) |
| `T81Quaternion` | `t81/core/T81Quaternion.hpp` | Quaternion for 3D/4D geometry. | Pair of `T81Complex<27>` (`w+xi`, `y+zi`) |
| `T81Prob<Trits>` | `t81/core/T81Prob.hpp` | Log-odds probability. | `T81Int<Trits>` (fixed-point log-odds) |
| `T81Polynomial<C>` | `t81/core/T81Polynomial.hpp` | Polynomial with coefficients `C`. | `T81List<C>` (coefficients) |
| `T81Symbolic` | `t81/core/T81Symbolic.hpp` | Symbolic expression tree node. | `std::shared_ptr<Expr>` (AST node variant) |

## Symbolic & Raw Data

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `T81Symbol` | `t81/core/T81Symbol.hpp` | Eternal, unique 81-trit identifier. | `T81Int<81>` (interned unique ID) |
| `T81String` | `t81/core/T81String.hpp` | Normalized ASCII/Ternary text string. | `std::string` (normalized subset `[A-Z ]`) |
| `T81Bytes` | `t81/core/T81Bytes.hpp` | Canonical byte buffer. | `std::vector<std::uint8_t>` |
| `Base81String` | `t81/core/base81.hpp` | Base-81 textual encoding. | `std::string` alias (checked encoding) |

## Containers & Structures

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `T81List<E>` | `t81/core/T81List.hpp` | Dynamic ternary-native list. | `std::vector<E>` (aligned) |
| `T81Map<K,V>` | `t81/core/T81Map.hpp` | Associative map (open addressing). | `std::vector<Bucket>` (Bucket = Key+Value+Occupied) |
| `T81Set<T>` | `t81/core/T81Set.hpp` | Immutable set. | `T81Map<T, std::monostate>` |
| `T81Tree<T>` | `t81/core/T81Tree.hpp` | Immutable ternary tree (shared structure). | `std::shared_ptr<Node>` (Node = Value + 3x Ptr) |
| `T81Graph` | `t81/core/T81Graph.hpp` | Static cache-oblivious graph. | `AdjacencyStorage` (`vector` or `array` of edge lists) |
| `T81Stream<T>` | `t81/core/T81Stream.hpp` | Infinite lazy sequence. | `std::coroutine_handle<promise_type>` |

## Numerical Containers

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `T81Vector<N,S>` | `t81/core/T81Vector.hpp` | Fixed-dimension geometric vector. | `S[N]` (aligned array of scalars) |
| `T81Matrix<S,R,C>` | `t81/core/T81Matrix.hpp` | Fixed-size matrix. | `S[R * C]` (row-major aligned array) |
| `T81Tensor<...>` | `t81/core/T81Tensor.hpp` | N-dimensional tensor. | `std::vector<E>` or `E[]` (contiguous storage) |
| `DistributedT81Tensor` | `t81/core/DistributedTensor.hpp` | Distributed tensor wrapper. | `std::unique_ptr<LocalTensor>` + shard metadata |
| `T729Tensor` | `t81/core/T729Tensor.hpp` | Holotensor (Base-729 domain). | Alias for `T81Tensor<T81Float<72,9>, ...>` |

## Semantic & Flow Control

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `Option<T>` | `t81/core/Option.hpp` | Language-facing optional alias. | Alias for `T81Maybe<T>` |
| `Result<T>` | `t81/core/Result.hpp` | Language-facing result alias. | Alias for `T81Result<T>` |
| `T81Maybe<T>` | `t81/core/T81Maybe.hpp` | Optional value (Maybe monad). | `std::optional<T>` wrapper |
| `T81Result<T>` | `t81/core/T81Result.hpp` | Success/Failure variant. | `std::variant<T, T81Error>` |
| `T81Promise<T>` | `t81/core/T81Promise.hpp` | Asynchronous promise (coroutine). | `std::coroutine_handle` wrapper |

## Civilizational & Reflective Types

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `T81Agent` | `t81/core/T81Agent.hpp` | Cognitive entity state. | Identity, beliefs (Map), memory (Tree), intent (Quat) |
| `T81Entropy` | `t81/core/T81Entropy.hpp` | Move-only entropy token. | `T81Int<81>` + Source + Sequence ID |
| `T81Time` | `t81/core/T81Time.hpp` | Logical timestamp. | `std::chrono::steady_clock::time_point` + Event ID |
| `T81IOStream` | `t81/core/T81IOStream.hpp` | Auditable I/O channel. | `FILE*` handle + path + history log |
| `T81Thread` | `t81/core/T81Thread.hpp` | Reflective thread wrapper. | `std::shared_ptr<ThreadState>` (`std::thread` + metadata) |
| `T81Network` | `t81/core/T81Network.hpp` | Network subsystem singleton. | `asio::io_context` + runner thread |
| `T81Discovery` | `t81/core/T81Discovery.hpp` | Peer discovery service. | UDP socket + threads + peer set |
| `T81Qutrit` | `t81/core/T81Qutrit.hpp` | Ternary quantum state unit. | Alias for `T81Int<2>` |
| `T81Category` | `t81/core/T81Category.hpp` | Category theory structure. | Sets of Objects + Map of Morphisms |
| `T81Proof` | `t81/core/T81Proof.hpp` | Formal verification proof. | Goal (Tree) + Steps list + Audit trail |
| `T81Reflection` | `t81/core/T81Reflection.hpp` | Reflection metadata wrapper. | Value + Type Symbol + Instance ID + Tags |

## Neural Network & AI

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `T81NN` | `t81/core/T81NN.hpp` | Neural network primitives. | Layers (Linear, Conv2d) wrapping `T81Tensor` weights |

## Low-Level Utilities

| Data Type | Header | Description | Representation |
|---|---|---|---|
| `Cell` | `t81/core/cell.hpp` | 5-trit balanced ternary cell. | `std::array<Trit, 5>` |
| `CellPacked` | `t81/core/cell_packed.hpp` | Packed cell storage. | `uint8_t` (Index 0..242) |
| `CanonicalId` | `t81/core/ids.hpp` | Canonical identifier. | `Base81String` (alias for `std::string`) |
| `bigint` | `t81/core/bigint.hpp` | Legacy bigint compatibility. | Facade over `t81::T81BigInt` |
| `fraction` | `t81/core/fraction.hpp` | Legacy fraction compatibility. | Struct with `BigInt` numerator, denominator |
| `tensor` | `t81/core/tensor.hpp` | Legacy tensor compatibility. | Struct with `vector<size_t>` shape, `vector<T>` data |
| `packing` | `t81/core/packing.hpp` | Packing utilities. | Helper functions (no storage) |
