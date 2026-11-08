# T81 SCG-05 — Generic Result Contract Feasibility Assessment

**Document ID:** T81-SCG-05-FEASIBILITY-001
**Baseline Git SHA:** `e45c5435aa47d4ccb8f343798214eb917352c871`
**Governing Policy:** SCG-04-MDR-001, Decision D4=4B (Adopt True Generic `Result<T, E>` Sum Type)
**Disposition:** READ-ONLY FEASIBILITY ASSESSMENT (NO PRODUCTION CODE CHANGES; NO SPECIFICATION CHANGES)

---

## 1. Executive Summary

Maintainer Decision Record SCG-04-MDR-001 recorded the architectural policy choice for Decision D4:
- **D4 = 4B:** Adopt a genuine generic `Result<T, E>` sum type where the error type `E` is semantically meaningful and domain-specific, rather than restricting all operations to a single-parameter `T81Result<T>` with fixed `T81Error`.
- **Implementation Status:** NOT GRANTED. Implementation authorization was explicitly withheld pending an independent feasibility assessment.

This report delivers the technical feasibility assessment mandated under SCG-05. It determines the minimum compatible architecture for introducing a generic `Result<T, E>` container while preserving existing `T81Result<T>` callers, maintaining full backward compatibility, respecting determinism constraints (`spec/determinism-profile.md`), and preserving all historical evidence receipts.

### Key Conclusions
1. **Additive Container Strategy:** The optimal strategy is an additive generic container `template <typename T, typename E = T81Error> class Result;` in `include/t81/types/Result.hpp`, while preserving `include/t81/types/T81Result.hpp` as a stable legacy compatibility wrapper or specialization alias `template <typename T> using T81Result = Result<T, T81Error>;`.
2. **Standard Library Compatibility (`std::expected` vs `std::variant`):** The repository toolchain supports C++20 as baseline and C++23 when `T81_USE_CXX23=ON` (enabled by default in `cmake/Options.cmake`). `std::expected<T, E>` is supported in C++23 (`gcc >= 12`, `clang >= 16`), with a proven deterministic fallback shim already in `include/t81/support/expected.hpp`. Backing `Result<T, E>` with `std::expected` (or direct standard-layout `std::variant<T, E>`) enables zero-allocation, `constexpr`-capable error propagation for lightweight error types (e.g., enums or error codes).
3. **`T81Error` Remediation vs Encapsulation:** Existing `T81Error` captures dynamic system state (`T81Time::now()` and `acquire_entropy()`), allocating heap memory and violating strict determinism (`spec/determinism-profile.md §6`). Generic `Result<T, E>` allows deterministic kernel and math libraries to use lightweight, deterministic error types (e.g. `enum class MathError`, `enum class CodecError`, or `std::error_code`) without modifying `T81Error` itself.
4. **Zero Production Risk Under Current Baseline:** Because all 16 existing caller files in `include/` and `tests/` rely on `Result<T>` or `T81Result<T>` with implicit or explicit `T81Error`, defaulting `E = T81Error` provides 100% source-level backward compatibility for existing code.

---

## 2. Source Inventory: Existing `Result` and `T81Result` Callers

An exhaustive audit of the authoritative baseline (`e45c5435aa47d4ccb8f343798214eb917352c871`) identified **18 files** across `include/`, `tests/`, and documentation referencing `Result<T>`, `T81Result<T>`, or `T81Error`.

### 2.1 File Inventory and Caller Taxonomy

| Subsystem / Layer | Header / Test File | References | Current Type Used | Methods Used | Error Handling Assumptions |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Surface Definition** | `include/t81/types/Result.hpp` | 2 | `Result<T> = T81Result<T>` | Single template parameter | Fixed error type `T81Error`. |
| **Core Container** | `include/t81/types/T81Result.hpp` | 27 | `T81Result<T>` | `success`, `failure`, `value`, `error`, `map`, `and_then`, `or_else`, `reflect`, `unwrap`, `expect` | Encapsulates `std::variant<T, T81Error>`. |
| **SWAR Acceleration** | `include/t81/swar/swar.hpp` | 35 | `Result<T>` | `success`, `failure`, `is_err`, `error().code`, `error().message`, `error().source` | Forwards `T81Symbol` code, message, and source. |
| **Tritwise Logic** | `include/t81/tritwise/tritwise.hpp` | 12 | `Result<bool>`, `Result<ComputeTritVector>` | Implicit propagation | Relies on `ComputeTritVector` member methods. |
| **SIMD Operations** | `include/t81/simd/simd.hpp` | 9 | `Result<ComputeTritVector>`, `Result<bool>` | Forwards return of `input.t_not()`, etc. | Direct pass-through of `Result<T>`. |
| **Trit Vector Representation** | `include/t81/experimental/packed_trit_vector.hpp` | 97 | `Result<PackedTritVector>`, `Result<bool>`, etc. | `success`, `failure`, `is_err`, `value`, `error` | Constructs failures via `failure(symbol, msg, src)`. |
| **Balanced Codec** | `include/t81/codec/base81_balanced.hpp` | 6 | `Result<size_t>`, `Result<vector<int8_t>>`, `Result<vector<Trit>>` | Declarations | Expects success with count or vector, or error. |
| **Trit Packing Codec** | `include/t81/codec/trit_packing.hpp` | 7 | `Result<vector<uint8_t>>`, `Result<vector<Trit>>` | Declarations | Same as balanced codec. |
| **Metadata Codec** | `include/t81/codec/metadata.hpp` | 1 | `Result<pair<Header, vector<uint8_t>>>` | Declarations | Same as balanced codec. |
| **CanonFS Storage Driver** | `include/t81/canonfs/canon_driver.hpp` | 5 | `Result<T> = std::expected<T, Error>` | `std::expected` methods | **Local alias:** Shadows global `Result`! Uses `std::expected<T, Error>` where `Error` is enum. |
| **Experimental Hanoi Kernel** | `include/t81/experimental/hanoi/kernel.hpp` | 17 | `Result<T> = std::expected<T, Error>` | `std::expected` methods | **Local alias:** Shadows global `Result`! Uses `std::expected<T, hanoi::Error>`. |
| **Experimental Cog** | `include/t81/experimental/cog/promotion.hpp` | 1 | `Result<T> = std::expected<T, PromotionError>` | `std::expected` methods | **Local alias:** Shadows global `Result`! Uses `std::expected<T, PromotionError>`. |
| **TISC Program View** | `include/t81/isa/base81_view.hpp` | 1 | `Result<Program>` | Declarations | Returns parsed program or failure. |
| **Async / Promises** | `include/t81/types/T81Promise.hpp` | 7 | `T81Result<T>` | `success`, `failure`, `error->code`, `error->message` | Directly accesses error fields via pointer. |
| **Networking** | `include/t81/types/T81Network.hpp` | 13 | `T81Result<socket>`, `T81Result<bool>`, `T81Result<T81Bytes>` | `success`, `failure` | Constructs `failure` with `errors::OUT_OF_ENTROPY`. |
| **Types Documentation** | `include/t81/types/README.md` | 4 | Documentation | Mentions `Result<T,E>` and `T81Result<T,E>` | Documents intended 2-parameter type. |
| **Comprehensive Test** | `tests/cpp/comprehensive_test.cpp` | 3 | `T81Result<T81Int<81>>` | `success`, `failure`, `operator bool` | Asserts `!bad` on division by zero. |
| **Unit Test** | `tests/cpp/test_T81Result.cpp` | 5 | `T81Result<T81Int<27>>` | `is_ok`, `is_err`, `unwrap`, `unwrap_or`, `map`, `and_then`, `error().code` | Verifies success and failure paths. |

### 2.2 Critical Discovery: Local Subsystem `Result<T>` Shadowing
A critical finding from the inventory is that subsystems requiring lightweight, non-allocating, and deterministic error propagation (specifically **CanonFS**, **Hanoi Kernel**, and **Cog Promotion**) were **unable to use `t81::Result<T>` / `t81::T81Result<T>`** due to `T81Error`'s dynamic overhead. Instead, each subsystem declared its own local alias:
- In `t81::canonfs`: `template <typename T> using Result = std::expected<T, Error>;`
- In `t81::hanoi`: `template <typename T> using Result = std::expected<T, Error>;`
- In `t81::cog`: `template <typename T> using Result = std::expected<T, PromotionError>;`

This proves empirically that the absence of a genuine generic `t81::Result<T, E>` forced developers to fracture the codebase into ad-hoc local aliases over `std::expected`. A unified generic `Result<T, E>` directly resolves this architectural fragmentation.

---

## 3. Analysis of `T81Error` Infrastructure

The current error representation is defined in `include/t81/types/T81Result.hpp`:

```cpp
struct T81Error {
  T81Symbol code;
  T81String message;
  T81Time occurred_at;
  T81Entropy fuel_spent;
  T81Symbol source;
  ...
};
```

### 3.1 Characteristics and Non-Determinism Profile
1. **Host Timestamp Acquisition:** The constructor calls `occurred_at(T81Time::now(src))`. `T81Time::now()` reads `std::chrono::steady_clock::now()` unless a static `deterministic_override_` is set under a global mutex. This violates `spec/determinism-profile.md §6`:
   > *"Error constructors MUST NOT capture stack traces with host pointers, timestamps, or thread IDs."*
2. **Entropy Consumption:** The constructor calls `fuel_spent(acquire_entropy())`. `acquire_entropy()` modifies a global atomic counter and executes a PRNG (`EntropyPool::hardware_trng()`). Creating an error mutates global entropy state.
3. **Copy Semantics Entropy Tax:** The copy constructor `T81Error(const T81Error& other)` captures fresh entropy and timestamp (`occurred_at(T81Time::now("ERROR_COPY"))`). Copying an error changes its timestamp and spends entropy.
4. **Dynamic Memory Allocation:** `message` is a `T81String` (which manages dynamic heap memory). Consequently, constructing or copying `T81Error` performs heap allocation, precluding its use in freestanding environments (`ternaryos/`), bare-metal kernel modes, or realtime SWAR loops.
5. **Exception and Constexpr Limitations:** Methods like `value()`, `unwrap()`, and `error()` throw `std::runtime_error` or `std::logic_error`. Although `T81Result<T>` constructors are marked `constexpr`, instantiating them with `T81Error` cannot be evaluated at compile time due to heap allocations and dynamic clock calls.

### 3.2 Architectural Consequence
Modifying `T81Error` directly to strip its timestamp, entropy, or message would break legacy code, introspection tooling, and tests that expect these diagnostic properties. Therefore, `T81Error` must remain untouched as the diagnostic error payload for high-level operations, while the result container itself must be parameterized by `E` so that low-level and deterministic code can supply lightweight, zero-allocation error types (e.g. `enum class MathError : int8_t`).

---

## 4. Evaluation of Candidate Architectures & Standards

### 4.1 Candidate Architectures

#### Architecture A: In-Place Modification of `T81Result<T>`
- **Description:** Turn `T81Result<T>` into `template <typename T, typename E = T81Error> class T81Result;` and make `Result<T, E>` an alias.
- **Pros:** Single container type in the codebase.
- **Cons:** High churn risk; any template signature mismatch or subtle method difference immediately breaks existing callers; tightly couples legacy names with modern vocabulary.

#### Architecture B: Additive Generic `Result<T, E>` (Recommended)
- **Description:** Implement a clean, modern, generic container `template <typename T, typename E = T81Error> class Result;` in `include/t81/types/Result.hpp`. Retain `T81Result<T>` in `include/t81/types/T81Result.hpp` as a compatibility wrapper or alias `template <typename T> using T81Result = Result<T, T81Error>;`.
- **Pros:**
  - Preserves exact ABI and API behavior for existing `T81Result<T>` callers.
  - Zero disruption to legacy tests or callers.
  - Clean separation between core vocabulary (`Result<T, E>`) and legacy types (`T81Result<T>`).
  - Allows progressive opt-in migration of subsystems (CanonFS, Hanoi, SWAR).
- **Cons:** Requires clear documentation to avoid developer confusion between `Result` and `T81Result`.

### 4.2 Standards and Backing Storage: `std::expected` vs `std::variant`

| Metric / Consideration | Option 1: Backed by `std::variant<T, E>` | Option 2: Backed by `std::expected<T, E>` | Option 3: Dedicated Storage Struct |
| :--- | :--- | :--- | :--- |
| **C++20 Compliance** | Native `<variant>` supported across all compilers. | Requires `<expected>` (C++23) or fallback shim (`include/t81/support/expected.hpp`). | Fully custom union/storage. |
| **C++23 Compliance** | Fully supported. | First-class standard vocabulary type. | Fully supported. |
| **T == E Disambiguation** | Requires tagged wrappers (e.g. `Err<E>`) to avoid ambiguous constructors when `T` and `E` are the same type. | Handled cleanly via `std::unexpected<E>`. | Handled via custom tag or state flag. |
| **Constexpr Evaluation** | Fully `constexpr` in C++20 for literal types. | Fully `constexpr` in C++23. | Fully `constexpr`. |
| **Standard Interop** | Requires custom conversion to `std::expected`. | Trivial / zero-cost conversion to `std::expected`. | Requires conversion shims. |
| **Recommendation** | Excellent fallback. | **Primary candidate** via existing repo abstraction `t81::expected`. | Unnecessary maintenance burden. |

**Synthesis:** In `include/t81/support/expected.hpp`, the repository already maintains a dual-mode `expected<T, E>` that uses standard `<expected>` under C++23 and a robust `std::variant`-backed shim under C++20. Utilizing this existing foundation for `Result<T, E>` provides maximum portability, zero new dependencies, and complete toolchain compliance.

---

## 5. Proposed API Contract for Generic `Result<T, E>`

The recommended contract for `t81::Result<T, E = T81Error>` provides standard monadic semantics while maintaining backward compatibility with `T81Result<T>` methods.

### 5.1 Class Template Definition

```cpp
namespace t81 {

template <typename E>
struct Err {
  E value;
  constexpr explicit Err(E v) : value(std::move(v)) {}
};

template <typename T, typename E = T81Error>
class Result {
public:
  using value_type = T;
  using error_type = E;

  // Constructors
  constexpr Result(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>);
  constexpr Result(T&& val) noexcept(std::is_nothrow_move_constructible_v<T>);
  constexpr Result(Err<E> err) noexcept(std::is_nothrow_move_constructible_v<E>);

  // Specific constructor for default E = T81Error compatibility
  template <typename E2 = E, typename std::enable_if_t<std::is_same_v<E2, T81Error>, int> = 0>
  constexpr Result(const T81Error& err) noexcept;

  template <typename E2 = E, typename std::enable_if_t<std::is_same_v<E2, T81Error>, int> = 0>
  constexpr Result(T81Error&& err) noexcept;

  // Factory methods
  static constexpr Result success(T val) noexcept;
  static constexpr Result failure(E err) noexcept;

  // Legacy failure factory for T81Error
  template <typename... Args, typename E2 = E,
            typename std::enable_if_t<std::is_same_v<E2, T81Error>, int> = 0>
  static constexpr Result failure(T81Symbol code, T81String msg, T81Symbol src = symbols::unk) noexcept;

  // State inspection
  [[nodiscard]] constexpr bool is_ok() const noexcept;
  [[nodiscard]] constexpr bool is_err() const noexcept;
  [[nodiscard]] constexpr explicit operator bool() const noexcept;

  // Value accessors
  [[nodiscard]] constexpr const T& value() const&;
  [[nodiscard]] constexpr T& value() &;
  [[nodiscard]] constexpr T&& value() &&;
  [[nodiscard]] constexpr const T& unwrap() const&;
  [[nodiscard]] constexpr T value_or(const T& fallback) const& noexcept;
  [[nodiscard]] constexpr T unwrap_or(const T& fallback) const& noexcept;
  [[nodiscard]] constexpr T expect(std::string_view msg) const;

  // Error accessors
  [[nodiscard]] constexpr const E& error() const&;
  [[nodiscard]] constexpr E& error() &;
  [[nodiscard]] constexpr E&& error() &&;

  // Monadic combinators
  template <typename F>
  [[nodiscard]] constexpr auto map(F&& f) const&;

  template <typename F>
  [[nodiscard]] constexpr auto and_then(F&& f) const&;

  template <typename F>
  [[nodiscard]] constexpr Result or_else(F&& f) const&;

  template <typename F>
  [[nodiscard]] constexpr auto transform_error(F&& f) const&;
};

}  // namespace t81
```

### 5.2 Key Properties
1. **Value & Error Semantics:** Supports move-only types for both `T` and `E`.
2. **`constexpr` Execution:** Fully usable in `constexpr` functions whenever `T` and `E` are literal types.
3. **`noexcept` Guarantees:** Move constructors and state queries are conditionally `noexcept` based on type traits.
4. **Deterministic Arithmetic Support:** Can be parameterized with zero-overhead error enums (e.g. `Result<T81Int<N>, ArithmeticError>`), satisfying both SCG-04 Decision D1 and D2 requirements without dynamic allocation.

---

## 6. Compatibility & Migration Impact Assessment

### 6.1 Call-Site Compatibility
- **Existing Callers:** All existing callers using `Result<T>` continue to resolve to `Result<T, T81Error>` via the default template parameter.
- **Factory Methods:** Existing calls to `Result<T>::success(...)` and `Result<T>::failure(...)` remain 100% syntactically and semantically identical.
- **Legacy Accessors:** Existing calls to `.is_ok()`, `.is_err()`, `.value()`, `.error()`, `.unwrap()`, `.unwrap_or()`, and `operator bool()` compile without modification.

### 6.2 Subsystem Migration Path
1. **Phase 1 (Introduction):** Add `Result<T, E = T81Error>` in `include/t81/types/Result.hpp`. Re-alias `T81Result<T>` to `Result<T, T81Error>`. All existing tests pass unchanged.
2. **Phase 2 (Subsystem Unification):** Migrate local aliases in `canonfs`, `hanoi`, and `cog` from `using Result = std::expected<T, SubsystemError>` to `using Result = t81::Result<T, SubsystemError>`.
3. **Phase 3 (Deterministic Arithmetic Bridges):** Under a future authorized D1/D2 task, provide checked arithmetic functions returning `Result<T81Int<N>, ArithmeticError>` and `Result<T81Float<M, E>, ConversionError>`.

---

## 7. Test and Acceptance Matrix

When future implementation authorization is granted, verification must adhere to the following matrix:

| Test ID | Test Category | Scenario | Expected Behavior | Stop Condition |
| :--- | :--- | :--- | :--- | :--- |
| **TEST-RES-01** | Compatibility | Instantiate `Result<int>` without second parameter | Defaults to `E = T81Error`; behaves identically to legacy `T81Result<int>`. | Any compilation error in existing tests. |
| **TEST-RES-02** | Generic Types | Instantiate `Result<int, enum Error>` | Uses enum error; sizeof equals `sizeof(int) + sizeof(enum) + alignment`. | Heap allocation or dependency on `T81Error`. |
| **TEST-RES-03** | Compile-time | `constexpr` evaluation of `Result<int, int>` | `static_assert(Result<int, int>::success(5).value() == 5);` compiles cleanly. | Compiler error during constant evaluation. |
| **TEST-RES-04** | Disambiguation | `Result<int, int>` with identical `T` and `E` | `Result::success(1)` vs `Result::failure(Err(2))` correctly distinguished. | Ambiguous overload resolution. |
| **TEST-RES-05** | Move-Only | `Result<std::unique_ptr<int>, CustomError>` | Value can be moved in and out without copying. | Compilation failure on move-only types. |
| **TEST-RES-06** | Monadic Chain | `.map()`, `.and_then()`, `.transform_error()` | Correctly transforms value or error type; preserves failure through chain. | Type deduction failure or incorrect state. |
| **TEST-RES-07** | Legacy Gate | Run full test suite (`test_T81Result`, `comprehensive_test`, SWAR tests) | 100% existing tests pass with zero regressions. | Any existing test failure. |

---

## 8. Unresolved Questions & Implementation Pre-conditions

### 8.1 Technical Questions for Implementation Phase
1. **`std::expected` Interoperability:** Should `Result<T, E>` inherit from or implicitly convert to `std::expected<T, E>`, or provide an explicit `.to_expected()` conversion method?
   - *Recommendation:* Provide explicit conversion `.to_expected()` and explicit constructor from `std::expected` to avoid ambiguous implicit conversions.
2. **Reflect API:** `T81Result<T>` includes a `.reflect()` method returning `T81Reflection<T81Result<T>>`. Should `.reflect()` be conditionally enabled via SFINAE only when `E` is reflectable?
   - *Recommendation:* Enable `.reflect()` conditionally via `requires` or `std::enable_if_t` to allow non-reflectable error types.

### 8.2 Scope for Future Implementation Authorization
Authorization to proceed with implementation must be granted by the maintainer via an explicit authorization directive specifying:
- **Scope Limit:** Implementation restricted to `include/t81/types/Result.hpp`, `include/t81/types/T81Result.hpp`, and a dedicated unit test suite `tests/cpp/test_generic_result.cpp`.
- **Zero Production Churn:** Existing callers in `swar`, `codec`, `canonfs`, and `isa` must not be force-migrated in the initial implementation PR; migration occurs in follow-up scoped changes.
- **CI Gates:** Verification must pass `clang-format-18`, GCC 13 C++20/C++23 builds, and the existing CTest suite.

---

## 9. Baseline Attestation and Sign-Off

- **Baseline SHA:** `e45c5435aa47d4ccb8f343798214eb917352c871`
- **Assessment Scope:** Read-only feasibility investigation. Zero production files modified.
- **Maintainer Policy Alignment:** SCG-04-MDR-001, Decision D4=4B.
