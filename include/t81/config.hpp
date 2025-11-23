#pragma once

// t81/config.hpp — feature toggles and portable attributes

// ------------------------------
// Version
// ------------------------------
#define T81_VERSION_MAJOR 1
#define T81_VERSION_MINOR 0
#define T81_VERSION_PATCH 0

// Compose as integer 0xMMmmpp (for simple #if checks)
#define T81_VERSION_HEX ((T81_VERSION_MAJOR<<16) | (T81_VERSION_MINOR<<8) | (T81_VERSION_PATCH))

// ------------------------------
// Build switches (override via -D at compile time)
// ------------------------------

// Enable additional runtime checks in debug builds.
#ifndef T81_ENABLE_ASSERTS
  #if !defined(NDEBUG)
    #define T81_ENABLE_ASSERTS 1
  #else
    #define T81_ENABLE_ASSERTS 0
  #endif
#endif

// Placeholder-encoding guard: when 1, BigInt::from_ascii uses the stub mapping;
// when 0, your platform must provide a real Base-243/81 conversion layer.
#ifndef T81_BASE243_PLACEHOLDER_ENCODING
#define T81_BASE243_PLACEHOLDER_ENCODING 1
#endif

// Allow very naive algorithms in tests (e.g., mod/gcd fallbacks).
#ifndef T81_ALLOW_NAIVE_ALGOS
#define T81_ALLOW_NAIVE_ALGOS 1
#endif

// ------------------------------
// Attributes / portability
// ------------------------------
#if defined(_MSC_VER)
  #define T81_FORCE_INLINE __forceinline
#else
  #define T81_FORCE_INLINE inline __attribute__((always_inline))
#endif

// Likely/unlikely hints
#if defined(__GNUC__) || defined(__clang__)
  #define T81_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define T81_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define T81_LIKELY(x)   (x)
  #define T81_UNLIKELY(x) (x)
#endif
