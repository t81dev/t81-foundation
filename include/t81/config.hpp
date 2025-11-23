#pragma once

// ---------- Version ----------
#define T81_VERSION_MAJOR 1
#define T81_VERSION_MINOR 1
#define T81_VERSION_PATCH 0
#define T81_VERSION_STR   "1.1.0"

// ---------- Feature toggles (edit as needed) ----------
// Enable lightweight internal assertions in headers (abort on failure)
#ifndef T81_ENABLE_ASSERTS
#define T81_ENABLE_ASSERTS 1
#endif

// Build with exceptions available (headers use throwing APIs in a few places)
#ifndef T81_USE_EXCEPTIONS
#define T81_USE_EXCEPTIONS 1
#endif

// ---------- Portability attributes ----------
#if defined(_MSC_VER)
  #define T81_FORCE_INLINE __forceinline
#else
  #define T81_FORCE_INLINE inline __attribute__((always_inline))
#endif

#if defined(__has_cpp_attribute)
  #if __has_cpp_attribute(nodiscard)
    #define T81_NODISCARD [[nodiscard]]
  #else
    #define T81_NODISCARD
  #endif
#else
  #define T81_NODISCARD
#endif

// ---------- Compiler hints ----------
#if defined(__GNUC__) || defined(__clang__)
  #define T81_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define T81_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define T81_LIKELY(x)   (x)
  #define T81_UNLIKELY(x) (x)
#endif

// ---------- Platform quirks ----------
#if defined(_MSC_VER)
  // MSVC lacks unsigned __int128 on some targets; gate typedefs accordingly.
  #if defined(_M_X64) || defined(_M_ARM64)
    #define T81_HAS_UINT128 1
  #else
    #define T81_HAS_UINT128 0
  #endif
#else
  #define T81_HAS_UINT128 1
#endif

// ---------- Sanity ----------
#if !T81_USE_EXCEPTIONS
  #error "Current headers assume exceptions are enabled (T81_USE_EXCEPTIONS=1)."
#endif
