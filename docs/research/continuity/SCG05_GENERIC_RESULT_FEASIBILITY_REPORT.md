# T81 SCG-05 — Generic Result Contract Feasibility Assessment

**Document ID:** T81-SCG-05-FEASIBILITY-001-R4
**Baseline Git SHA:** `e45c5435aa47d4ccb8f343798214eb917352c871`
**Rechecked against:** `2e6e78056e2b80855f06cd9a3690c4a65cb33d2a` (`origin/main` on 2026-11-23)
**Governing direction:** D4 option 4B, accepted as a non-normative direction by PM decision on 2026-11-23. The ballot in [`SCG04_MAINTAINER_DECISION_RECORD.md`](SCG04_MAINTAINER_DECISION_RECORD.md) remains unsigned. Implementation is unauthorized.
**Provisional name:** `t81::GenericResult<T, E>`, until implementation is authorized.
**Disposition:** READ-ONLY FEASIBILITY ASSESSMENT (NO PRODUCTION CODE CHANGES; NO SPECIFICATION CHANGES)

**Revision R3 (2026-11-23):** Corrects the caller-inventory scope, the status of Decision D4 as it stood before the PM decision, the C++20 `std::expected` fallback, the section 6 determinism quotation, and the unconditional heap-allocation claim for `T81String`.

**Revision R4 (2026-11-23):** Records the PM decision that D4 option 4B is the accepted non-normative direction, names `t81::GenericResult<T, E>` as provisional, and lists the deferred implementation questions. The SCG-04 decision record is unchanged. This revision still changes no production code and no normative specification.

---

## 1. Executive Summary

On 2026-11-23 the acting PM accepted Decision D4 option 4B as a **non-normative** direction: a genuine generic `Result<T, E>` sum type whose error type `E` is domain-specific. That acceptance matches the later SCG-07 receipts and `docs/architecture/visualization/research_programs.json`, which already cite D4=4B and state that implementation remains unauthorized. The original ballot in [`SCG04_MAINTAINER_DECISION_RECORD.md`](SCG04_MAINTAINER_DECISION_RECORD.md) is unsigned; its checkboxes are blank and its status line still says the instrument is pending maintainer signature. This report records the PM acceptance. It does not edit that historical ballot, it does not sign it, and it does not authorize an implementation. The label `SCG-04-MDR-001` is not an identifier in the decision record. The provisional public name, until implementation is authorized, is `t81::GenericResult<T, E>`.

This report is the SCG-05 feasibility assessment, revised under PR #732. It describes a compatible shape for a generic `Result<T, E>` container that leaves the existing `T81Result<T>` class unchanged, records ABI and compatibility risks, and checks the constraints in `spec/determinism-profile.md`.

### Key Conclusions
1. **Additive Container Strategy (Preserving Concrete `T81Result<T>`):** The accepted non-normative direction uses a distinct generic container under the provisional name `t81::GenericResult<T, E>` while **preserving the existing concrete `T81Result<T>` class in `include/t81/types/T81Result.hpp` completely unchanged**. That name stays provisional until implementation is authorized. Replacing the existing `T81Result<T>` class with a template alias would carry substantial risks of ABI breakage, compiler diagnostic shifts, deduction-guide disruption, and subtle overload resolution discrepancies across translation units. Keep the existing `t81::Result<T>` alias. Conversion of `T81Error` metadata is deferred; see the Deferred list in section 8.
2. **Toolchain Standard Distinction (`std::expected` vs C++20 `t81::expected`):** `cmake/Options.cmake` defaults `T81_USE_CXX23` to `ON`, which selects C++23 language mode, with C++20 as the fallback mode. `include/t81/support/expected.hpp` uses the ISO `std::expected` when `__cpp_lib_expected` is defined. Otherwise it defines `t81::expected` and publishes that type as `std::expected` with a `namespace std` alias. The fallback members are not declared `constexpr`. Its `value()` / `error()` behavior differs for `void` and non-`void` success types, and it does not throw `std::bad_expected_access`.
3. **`T81Error` Remediation vs Encapsulation:** Existing `T81Error` captures dynamic system state (`T81Time::now()` and `acquire_entropy()`) and stores its message in a `T81String` backed by `std::string`. That conflicts with `spec/determinism-profile.md` section 6. Generic `Result<T, E>` would let deterministic kernel and math libraries use lightweight error types (for example a named `enum class MathError : int8_t` or `enum class CodecError : uint8_t`) while leaving `T81Error` in place.
4. **Architectural Fragmentation in Existing Subsystems:** CanonFS, the experimental Hanoi kernel, and Cog promotion each declare a namespace-local `Result<T>` alias over `std::expected` and a subsystem error type. Those aliases are separate from `t81::Result`.
5. **Qualified Compatibility Boundaries:** While defaulting `E = T81Error` enables potential source-level convergence for simple call sites, caller coverage and compatibility across diverse translation units cannot be claimed as zero-risk without empirical compilation evidence.

---

## 2. Source Inventory: Existing `Result` and `T81Result` Callers

### 2.1 Search Methodology and Search Scope
The caller table below was prepared at commit `e45c5435aa47d4ccb8f343798214eb917352c871`. A fixed-string `git grep` of `Result<`, `T81Result<`, and `T81Error` was repeated on 2026-11-23 against that commit and against `5a005b2dea5e1eb262d4af45698f8338d16b137c`.
- **Search roots declared for the table:** `include/`, `src/`, `tools/`, `tests/`, and `spec/`.
- **Patterns searched:** `Result<`, `T81Result<`, and `T81Error`. A search for the substring `Result<` also matches `T81Result<`.
- **What the 18-row table covers:** C++ headers under `include/` and two tests under `tests/cpp/`. Reference counts in that table match matching lines. For `include/t81/types/T81Result.hpp`, 27 is the number of lines containing `Result<` or `T81Error` (10 lines contain `Result<`, 18 lines contain `T81Error`, and those sets overlap; the same file has 30 occurrences).
- **What that table omitted at the baseline:** `src/` and `tools/` had no matches. `spec/` had five additional files, listed in section 2.4. Tracked C++ sources outside the declared roots also match, including `codec/`, `fs/`, `experimental/hanoi/in_memory_kernel.cpp`, `experimental/tiers/cog/promotion.cpp`, `isa/base81_view.cpp`, `benchmarks/BM_DispatchOverhead.cpp`, and two `examples/` translation units. Dynamic macro expansion and code outside this repository were not searched.

### 2.2 Caller Taxonomy

The following **18** headers and tests are the original caller table. They are a header-and-test inventory, and section 2.4 records the other matches inside the declared roots.

| Subsystem / Layer | Header / Test File | References | Current Type Used | Methods Used | Error Handling Assumptions |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Surface Definition** | `include/t81/types/Result.hpp` | 2 | `template <typename T> using Result = T81Result<T>` | Single template parameter | The alias name is `Result`, defined as `T81Result<T>`. |
| **Core Container** | `include/t81/types/T81Result.hpp` | 27 | `T81Result<T>` | `success`, `failure`, `value`, `error`, `map`, `and_then`, `or_else`, `reflect`, `unwrap`, `expect` | Encapsulates `std::variant<T, T81Error>`. |
| **SWAR Acceleration** | `include/t81/swar/swar.hpp` | 35 | `Result<T>` | `success`, `failure`, `is_err`, `error().code`, `error().message`, `error().source` | Accesses `T81Symbol` code, message, and source from `T81Error`. |
| **Tritwise Logic** | `include/t81/tritwise/tritwise.hpp` | 12 | `Result<bool>`, `Result<ComputeTritVector>` | Implicit propagation | Relies on `ComputeTritVector` member methods. |
| **SIMD Operations** | `include/t81/simd/simd.hpp` | 9 | `Result<ComputeTritVector>`, `Result<bool>` | Forwards return of `input.t_not()`, etc. | Direct pass-through of `Result<T>`. |
| **Trit Vector Representation** | `include/t81/experimental/packed_trit_vector.hpp` | 97 | `Result<PackedTritVector>`, `Result<bool>`, etc. | `success`, `failure`, `is_err`, `value`, `error` | Constructs failures via `failure(symbol, msg, src)`. |
| **Balanced Codec** | `include/t81/codec/base81_balanced.hpp` | 6 | `Result<size_t>`, `Result<vector<int8_t>>`, `Result<vector<Trit>>` | Function declarations | Returns success with value, or error on failure. |
| **Trit Packing Codec** | `include/t81/codec/trit_packing.hpp` | 7 | `Result<vector<uint8_t>>`, `Result<vector<Trit>>` | Function declarations | Returns packed buffers or failure. |
| **Metadata Codec** | `include/t81/codec/metadata.hpp` | 1 | `Result<pair<Header, vector<uint8_t>>>` | Function declarations | Returns unwrapped header/buffer or failure. |
| **CanonFS Storage Driver** | `include/t81/canonfs/canon_driver.hpp` | 5 | `Result<T> = std::expected<T, Error>` | `std::expected` methods | **Namespace-local alias** in `t81::canonfs`. It names `std::expected` and the local `Error` enum. It does not change `t81::Result`. |
| **Experimental Hanoi Kernel** | `include/t81/experimental/hanoi/kernel.hpp` | 17 | `Result<T> = std::expected<T, Error>` | `std::expected` methods | **Namespace-local alias** in `t81::hanoi`. It names `std::expected` and `t81::hanoi::Error`. It does not change `t81::Result`. |
| **Experimental Cog** | `include/t81/experimental/cog/promotion.hpp` | 1 | `Result<T> = std::expected<T, PromotionError>` | `std::expected` methods | **Namespace-local alias** in `t81::cog`. It names `std::expected` and `PromotionError`. It does not change `t81::Result`. |
| **TISC Program View** | `include/t81/isa/base81_view.hpp` | 1 | `Result<Program>` | Function declarations | Returns parsed program or failure. |
| **Async / Promises** | `include/t81/types/T81Promise.hpp` | 7 | `T81Result<T>` | `success`, `failure`, `error->code`, `error->message` | Directly accesses error fields via pointer. |
| **Networking** | `include/t81/types/T81Network.hpp` | 13 | `T81Result<socket>`, `T81Result<bool>`, `T81Result<T81Bytes>` | `success`, `failure` | Constructs `failure` with `errors::OUT_OF_ENTROPY`. |
| **Types Documentation** | `include/t81/types/README.md` | 4 | Documentation | Mentions `Result<T,E>` and `T81Result<T,E>` | Historical documentation notes two-parameter sum type. |
| **Comprehensive Test** | `tests/cpp/comprehensive_test.cpp` | 3 | `T81Result<T81Int<81>>` | `success`, `failure`, `operator bool` | Asserts failure branch on division by zero. |
| **Unit Test** | `tests/cpp/test_T81Result.cpp` | 5 | `T81Result<T81Int<27>>` | `is_ok`, `is_err`, `unwrap`, `unwrap_or`, `map`, `and_then`, `error().code` | Unit verification for legacy container. |

### 2.3 Observed Architectural Fragmentation
Three headers declare namespace-local aliases:
- In `t81::canonfs`: `template <typename T> using Result = std::expected<T, Error>;`
- In `t81::hanoi`: `template <typename T> using Result = std::expected<T, Error>;`
- In `t81::cog`: `template <typename T> using Result = std::expected<T, PromotionError>;`

This is observed architectural fragmentation. The three aliases use domain-specific enumeration error types with the name `std::expected`. `T81Result<T>` binds its error type to `T81Error`. The source text does not state why the aliases were introduced. On the C++20 fallback path, `include/t81/support/expected.hpp` injects `template <typename T, typename E> using expected = ::t81::expected<T, E>;` into `namespace std`, so these aliases name the shim when `__cpp_lib_expected` is absent and the ISO type when that macro is set.

### 2.4 Matches Omitted from the 18-Row Table

At baseline `e45c5435`, the declared `spec/` root also contains:

| File | `Result<` lines | What the match is |
| :--- | ---: | :--- |
| `spec/companion/t81-spec.md` | 1 | A composite-type listing `Result<O,E>`. |
| `spec/rfcs/RFC-0040-swar-formalization.md` | 10 | SWAR signatures returning `Result<ComputeTritVector>` or `Result<bool>`. |
| `spec/rfcs/RFC-0041-simd-formalization.md` | 11 | SIMD signatures of the same shape. |
| `spec/rfcs/RFC-0042-deterministic-backend-equivalence-contract.md` | 1 | A bullet naming returned `Result<T>`. |
| `spec/supplemental/hanoi-kernel-spec.md` | 18 | Syscall signatures written as two-parameter `Result<T, HanoiError>`. |

The same baseline has C++ matches outside the declared roots: `codec/base81_balanced.cpp`, `codec/metadata.cpp`, `codec/trit_packing.cpp`, `fs/block_backed_driver.cpp`, `fs/in_memory_driver.cpp`, `fs/persistent_driver.cpp`, `experimental/hanoi/in_memory_kernel.cpp`, `experimental/tiers/cog/promotion.cpp`, `isa/base81_view.cpp`, `benchmarks/BM_DispatchOverhead.cpp`, `examples/ai-and-inference/ai-integration/complex_demo.cpp`, and `examples/compiler-and-ffi/complex_demo.cpp`.

Rechecked at `5a005b2d`, the 18 header and test rows above still have the same pattern counts. `tests/cpp/canonfs_policy_boundary_test.cpp` is new inside `tests/` and contains one `Result<void>` after `using namespace t81::canonfs`, so it uses the CanonFS alias. `examples/research/deterministic-risk-triage/risk_triage_gate.cpp` is an additional out-of-root file containing `T81Result`. The later `origin/main` commit `2e6e7805` does not modify those counted files and adds no `Result<`, `T81Result<`, or `T81Error` matches in its new CanonFS test or benchmark records.

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

  T81Error(T81Symbol c, T81String m, T81Symbol src = symbols::unk)
      : code(c),
        message(std::move(m)),
        occurred_at(T81Time::now(src)),
        fuel_spent(acquire_entropy()),
        source(src) {}

  T81Error(const T81Error& other)
      : code(other.code),
        message(other.message),
        occurred_at(T81Time::now(T81Symbol::intern("ERROR_COPY"))),
        fuel_spent(acquire_entropy()),
        source(other.source) {}

  [[nodiscard]] T81String explain() const { ... }
};
```

### 3.1 Characteristics and Determinism Profile
1. **Host Timestamp Acquisition:** The constructor calls `occurred_at(T81Time::now(src))`. `T81Time::now()` locks `override_mutex_` and returns `deterministic_override_` when it holds a value; otherwise it reads `std::chrono::steady_clock::now()`. This conflicts with `spec/determinism-profile.md` section 6:

   > Errors (`Result::Err(E)`) must be value types.
   >
   > * **Constructors**: Error constructors MUST NOT capture:
   >   * Stack traces with host pointers.
   >   * Timestamps.
   >   * Thread IDs.

2. **Entropy State Mutation:** The constructor calls `fuel_spent(acquire_entropy())`. `acquire_entropy()` calls `EntropyPool::acquire()`, which `fetch_add`s a global atomic sequence counter and then advances `seed_` under `seed_mutex_` (`seed_ ^= seed_ << 13`, then shifts of 7 and 17). Constructing an error mutates global entropy-pool state.
3. **Copy Semantics Entropy Cost:** The copy constructor `T81Error(const T81Error& other)` calls `T81Time::now(T81Symbol::intern("ERROR_COPY"))` and `acquire_entropy()`. Copying an error replaces its timestamp and consumes fresh entropy. `T81Error` declares that copy constructor and does not declare a move constructor, so an rvalue `T81Error` also uses the copy constructor.
4. **String Storage:** `message` is a `T81String`, and `T81String` stores a `std::string`. Messages that exceed the implementation's small-string buffer allocate. The type also depends on the hosted `std::string` facility, so it is a poor fit for an allocation-free or freestanding error code in `ternaryos/`.
5. **Exceptions and Constexpr:** `value()` and `unwrap()` throw `std::runtime_error` on the error state. `error()` throws `std::logic_error` on the success state. `T81Result<T>` constructors and `failure(code, msg, src)` are marked `constexpr` and `noexcept`, while `T81Error`'s constructor is neither `constexpr` nor `noexcept`. A constant-expression evaluation that constructs a `T81Error` is therefore unavailable, and a throw from that constructor during a `noexcept` `T81Result` operation would call `std::terminate`.

### 3.2 Preservation of `T81Error`
`T81Error` encapsulates diagnostic and narrative error information relied upon by existing tests, reflection machinery, and logging utilities. It should not be altered in place or broken. Instead, enabling a generic error parameter `E` allows systems requiring deterministic, zero-allocation error reporting to use lightweight error types (e.g. enum classes) while leaving `T81Error` intact for high-level operations.

---

## 4. Evaluation of Candidate Architectures & Standards

### 4.1 Candidate Architectures

#### Architecture A: Modifying `T81Result<T>` in Place (Rejected)
- **Description:** Turn `T81Result<T>` into `template <typename T, typename E = T81Error> class T81Result;` and make `Result<T, E>` an alias.
- **Risks:**
  - **High ABI & API Risk:** Changing the template arity or signature of `T81Result` risks breaking existing binary interfaces, template specializations, deduction guides, and forward declarations.
  - **Overload Ambiguity:** Modifying the existing class forces immediate reconciliation of all member methods with arbitrary `E`.
- **Verdict:** REJECTED.

#### Architecture B: Additive Generic `Result<T, E>` while Preserving Concrete `T81Result<T>` (Recommended)
- **Description:**
  - Introduce an additive generic container under the provisional name `t81::GenericResult<T, E>`. The name stays provisional until implementation is authorized.
  - **Retain the existing concrete `T81Result<T>` class in `include/t81/types/T81Result.hpp` completely unchanged.**
  - Preserve `t81::Result<T>` as the existing alias. Defer legacy conversions until the error metadata contract is decided.
- **Advantages:**
  - Keeps existing type identities intact by construction. Source and binary compatibility still require validation on the supported toolchains.
  - Leaves existing tests (`test_T81Result.cpp`, `comprehensive_test.cpp`) testing the exact legacy container without semantic drift.
  - Allows new code to adopt the generic container with domain-specific error types without coupling to `T81Error`.
  - Isolates interoperability to explicit converting boundaries.
- **Verdict:** RECOMMENDED.

### 4.2 Standards Evaluation: C++23 `std::expected` vs C++20 `t81::expected`

| Feature / Capability | ISO `std::expected<T, E>` when `__cpp_lib_expected` is defined | Repository fallback in `include/t81/support/expected.hpp` |
| :--- | :--- | :--- |
| **Availability** | Selected by the feature-test macro. `T81_USE_CXX23=ON` selects C++23 language mode and does not itself test the library. Libstdc++ commonly provides the type from GCC 12 and libc++ from LLVM 16. | Used when `__cpp_lib_expected` is absent. The header defines `t81::expected` and also aliases that name into `namespace std`. |
| **Semantic Completeness** | The ISO C++23 specification, including `bad_expected_access<E>`. | Partial shim. `and_then`, `transform`, `or_else`, and `transform_error` are declared for `&` and `const&` only. There are no `&&` overloads and no `value_or`. |
| **`constexpr` Support** | The ISO operations that are specified `constexpr` apply to literal types. | `t81::expected` members are not declared `constexpr`. The non-`void` specialization stores a `std::variant<T, E>`. |
| **Move-Only Support** | Required by the ISO wording for the operations that construct or move `T` and `E`. | Basic constructors accept rvalues. Move-only payloads through the monadic members still need a compile test. |
| **Exception Guarantees** | `value()` throws `std::bad_expected_access<E>` when there is no value. | Non-`void` `value()` and `error()` return `std::get` without a prior state check, so the wrong alternative throws `std::bad_variant_access`. `expected<void, E>::value()` is `noexcept` and returns without inspecting `has_`. `expected<void, E>::error()` returns `error_` even when `has_` is true. |

**Technical Finding:** Call sites written as `std::expected` observe the ISO type only when `__cpp_lib_expected` is defined. On the fallback path the same name is `t81::expected`. Backing a future `Result<T, E>` with that fallback requires compile tests before any claim of `constexpr` evaluation, move-only propagation, or monadic behavior. The fallback is not a substitute for `std::bad_expected_access`.

---

## 5. Proposed API Contract for Generic `Result<T, E>`

The sketch uses the provisional name `t81::GenericResult<T, E = T81Error>`. That name is provisional until implementation is authorized. The existing alias `t81::Result<T>` and a class template with the same qualified name cannot coexist. Giving the new class a default second template parameter would still change the identity of existing `t81::Result<T>` instantiations if the alias were replaced. Exception specifications and conditional `noexcept` clauses are defined conservatively to reflect the possibility that constructing, copying, moving, or transforming `T` or `E` may throw.

### 5.1 Class Template Definition

```cpp
namespace t81 {

// Disambiguation tag for error construction when T and E are identical or convertible
template <typename E>
struct Err {
  E value;
  constexpr explicit Err(E v) noexcept(std::is_nothrow_move_constructible_v<E>)
      : value(std::move(v)) {}
};

template <typename T, typename E = T81Error>
class GenericResult {
public:
  using value_type = T;
  using error_type = E;

  // Value constructors (conditional noexcept)
  constexpr GenericResult(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>);
  constexpr GenericResult(T&& val) noexcept(std::is_nothrow_move_constructible_v<T>);

  // Tagged error constructor
  constexpr GenericResult(Err<E> err) noexcept(std::is_nothrow_move_constructible_v<E>);

  // Copy and move constructors (conditional noexcept)
  constexpr GenericResult(const GenericResult& other) noexcept(
      std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_constructible_v<E>);
  constexpr GenericResult(GenericResult&& other) noexcept(
      std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E>);

  // Factory methods
  static constexpr GenericResult success(T val) noexcept(std::is_nothrow_move_constructible_v<T>);
  static constexpr GenericResult failure(E err) noexcept(std::is_nothrow_move_constructible_v<E>);

  // Legacy failure factory enabled ONLY when E == T81Error
  template <typename... Args, typename E2 = E,
            typename std::enable_if_t<std::is_same_v<E2, T81Error>, int> = 0>
  static GenericResult failure(T81Symbol code, T81String msg, T81Symbol src = symbols::unk);

  // State inspection (unconditionally noexcept)
  [[nodiscard]] constexpr bool is_ok() const noexcept;
  [[nodiscard]] constexpr bool is_err() const noexcept;
  [[nodiscard]] constexpr explicit operator bool() const noexcept;

  // Value accessors (may throw on error)
  [[nodiscard]] constexpr const T& value() const&;
  [[nodiscard]] constexpr T& value() &;
  [[nodiscard]] constexpr T&& value() &&;
  [[nodiscard]] constexpr const T& unwrap() const&;

  // Fallback accessors (conditional noexcept)
  [[nodiscard]] constexpr T value_or(const T& fallback) const& noexcept(
      std::is_nothrow_copy_constructible_v<T>);
  [[nodiscard]] constexpr T unwrap_or(const T& fallback) const& noexcept(
      std::is_nothrow_copy_constructible_v<T>);

  // Diagnostic accessors (may throw on error)
  [[nodiscard]] T expect(std::string_view msg) const;

  // Error accessors (may throw on success)
  [[nodiscard]] constexpr const E& error() const&;
  [[nodiscard]] constexpr E& error() &;
  [[nodiscard]] constexpr E&& error() &&;

  // Monadic combinators
  template <typename F>
  [[nodiscard]] constexpr auto map(F&& f) const&;

  template <typename F>
  [[nodiscard]] constexpr auto and_then(F&& f) const&;

  template <typename F>
  [[nodiscard]] constexpr GenericResult or_else(F&& f) const&;

  template <typename F>
  [[nodiscard]] constexpr auto transform_error(F&& f) const&;

  // Legacy conversions require a separate metadata contract.
};

}  // namespace t81
```

### 5.2 Exception Specification Principles
1. **Conditional Noexcept:** Move and copy operations MUST NOT be marked unconditionally `noexcept`; they must be conditionally `noexcept` based on the relevant copy or move construction traits of both `T` and `E`.
2. **Accessors on Mismatched State:** `.value()`, `.unwrap()`, and `.error()` access payload elements when present. When invoked in the wrong state (e.g., accessing `.value()` when `is_err()` is true), they throw exceptions (or abort in `-fno-exceptions` mode). These accessors are therefore **not** `noexcept`.
3. **Legacy Factory Operations:** The legacy `failure(code, msg, src)` method constructs a `T81Error`, which allocates heap memory (`T81String`) and calls system clocks; it is explicitly **not** `noexcept`.

---

## 6. Compatibility & Migration Impact Assessment

### 6.1 Preserving Concrete `T81Result<T>` vs Alias Risks
Replacing `T81Result<T>` with a template alias was evaluated and rejected due to significant technical risks:
- **ABI Mangling Discrepancies:** A concrete class `class T81Result` has a different mangled name than a template instantiation `Result<T, T81Error>`. Existing object files or shared libraries linked across compiler boundaries would experience symbol mismatches.
- **Template Deduction Guides:** Existing deduction guides for `T81Result` (e.g. `T81Result(T) -> T81Result<T>`) do not automatically transfer to aliased templates in all C++20 compilers without explicit alias deduction guides (P0608 / C++20 feature set variations).
- **Forward Declarations:** Any code forward-declaring `template <typename T> class T81Result;` fails compilation if `T81Result` is transformed into an alias `using T81Result = Result<T, T81Error>;`.

**Recommendation:** Retain `class T81Result<T>` as a distinct, unchanged concrete class in `include/t81/types/T81Result.hpp`. Retain the `t81::Result<T>` alias. The provisional name for the generic container is `t81::GenericResult<T, E>`. The current `T81Error` copy constructor obtains a new timestamp and fresh entropy. Both legacy error construction overloads can invoke that constructor, including the overload that accepts an rvalue. An owned error round trip through those paths therefore cannot promise preservation of `occurred_at` or `fuel_spent`. How a conversion handles all five fields is deferred; see section 8.

### 6.2 Phased Migration Strategy
1. **Phase 1 (Additive Specification & Container):** After implementation authorization confirms or replaces the provisional name `t81::GenericResult<T, E>`, implement the additive container and test it independently. Retain the legacy alias and concrete class.
2. **Phase 2 (Subsystem Evaluation):** Independently evaluate whether subsystems using local aliases (**CanonFS**, **Hanoi Kernel**, **Cog**) benefit from migrating to `t81::GenericResult<T, SubsystemError>`. Each migration is scoped and verified in an isolated PR.
3. **Phase 3 (Checked Arithmetic Integration):** When authorized under D1/D2, introduce checked arithmetic functions returning `GenericResult<T81Int<N>, ArithmeticError>` and `GenericResult<T81Float<M, E>, ConversionError>`, achieving zero-allocation arithmetic error handling.

---

## 7. Test and Acceptance Matrix

Future implementation verification must adhere to the following test matrix:

| Test ID | Test Category | Scenario | Portable Acceptance Criteria | Stop Condition |
| :--- | :--- | :--- | :--- | :--- |
| **TEST-RES-01** | Default Error Type | Instantiate `GenericResult<int>` | Compiles with `E = T81Error`. Value and error states correctly represented. | Compilation failure or incorrect default type. |
| **TEST-RES-02** | Lightweight Error Types | Declare `enum class ErrorCode : uint8_t { Overflow };` and instantiate `GenericResult<int, ErrorCode>` | Discriminates between success and error states. Preserves the exact error-code value. Makes no dynamic heap allocation for that trivial value/error pair. | Any dynamic heap allocation or failure to discriminate states. |
| **TEST-RES-03** | Compile-Time Evaluation | `constexpr` evaluation of `GenericResult<int, int>` | Evaluates in `constexpr` and `static_assert` contexts under supported toolchains. | Compiler error during constant expression evaluation. |
| **TEST-RES-04** | Disambiguated Construction | `GenericResult<int, int>` with identical value and error types | Successfully constructs ok state via `GenericResult::success(1)` and error state via `GenericResult::failure(2)` or the tagged constructor without ambiguity. | Ambiguous overload resolution or incorrect state. |
| **TEST-RES-05** | Move-Only Types | `GenericResult<std::unique_ptr<int>, CustomError>` | Move construction and move assignment operate without copy operations. | Compilation error on move-only types. |
| **TEST-RES-06** | Monadic Transformations | Chained invocations of `map`, `and_then`, `transform_error` | State and payload types transform correctly across the chain. Short-circuits on failure. | Type deduction failure or incorrect propagation. |
| **TEST-RES-07** | Legacy Interoperability | Candidate conversions between `GenericResult<T, T81Error>` and `T81Result<T>` | Deferred to implementation authorization (section 8). Compare `code`, `message`, `source`, `occurred_at`, and `fuel_spent` in both directions on any path advertised as lossless. | A path advertised as lossless changes any field, or conversion proceeds before that authorization. |
| **TEST-RES-08** | Legacy Regression Gate | Execute full existing test suite (`test_T81Result`, `comprehensive_test`) | 100% of existing tests continue to compile and pass against unchanged `T81Result<T>`. | Any regression or compilation failure in existing tests. |
| **TEST-RES-09** | Legacy Alias Identity | Include the proposed header alongside `t81/types/Result.hpp` | `std::is_same_v<t81::Result<int>, t81::T81Result<int>>` remains true, and the generic type can be named independently. | Conflicting declaration or changed legacy alias identity. |

---

## 8. Deferred Items and Implementation Pre-conditions

### Deferred

These three items stay deferred to the implementation-authorization step. This assessment does not decide them:

1. **`T81Error` conversion.** Whether `t81::GenericResult<T, T81Error>` and `T81Result<T>` convert, and how that conversion treats `occurred_at` and `fuel_spent`. The copy constructor replaces both fields.
2. **`-fno-exceptions` abort policy.** Which abort mechanism `.value()` and `.error()` use on the wrong state in freestanding or `-fno-exceptions` builds, including `ternaryos/`.
3. **`.reflect()`.** Whether `t81::GenericResult<T, E>` provides `.reflect()` when `E` does not implement `T81Reflection`.

### 8.1 Open Technical Question
**Fallback shim monadic support:** The current fallback already declares `and_then`, `transform`, `or_else`, and `transform_error` for `&` and `const&`. Which of those declarations, plus any future `&&` overloads, compile on the supported GCC and Clang versions is still untested.

### 8.2 Explicit Scope Limits for Future Implementation Authorization
Implementation remains unauthorized. A later authorization directive has to specify:
- **Provisional name:** `t81::GenericResult<T, E>` is the name recorded here. Authorization confirms that name or replaces it before any header is added. Preserve the existing `Result<T>` alias.
- **Deferred list:** Resolve the three deferred items above in that same authorization, before code that depends on them is written.
- **Zero Modifications to Concrete Classes:** `include/t81/types/T81Result.hpp` MUST remain unchanged.
- **Zero Modifications to Existing Subsystems:** No changes to `canonfs`, `hanoi`, `cog`, `swar`, or `codec` call sites in the initial PR.
- **Validation Gates:** Full passage of `clang-format-18`, C++20 and C++23 build targets, and existing CTest regression gates.

---

## 9. Baseline Attestation and Sign-Off

- **Baseline SHA:** `e45c5435aa47d4ccb8f343798214eb917352c871`
- **Recheck SHA:** `2e6e78056e2b80855f06cd9a3690c4a65cb33d2a`
- **Assessment Scope:** Read-only feasibility investigation. Zero production files modified. The SCG-04 decision record is unchanged.
- **Policy:** D4 option 4B is the accepted non-normative direction by PM decision on 2026-11-23. The ballot in `docs/research/continuity/SCG04_MAINTAINER_DECISION_RECORD.md` remains unsigned. Implementation is unauthorized.
- **Provisional name:** `t81::GenericResult<T, E>`.
- **Deferred:** `T81Error` conversion of `occurred_at` and `fuel_spent`; the `-fno-exceptions` abort policy; `.reflect()`.
