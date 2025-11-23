#pragma once
#include <cstdlib>
#include <cstdio>
#include "t81/config.hpp"

// Lightweight assertion macros for t81 internals.
// Controlled by T81_ENABLE_ASSERTS (see config.hpp).

#if T81_ENABLE_ASSERTS
  #define T81_ASSERT(cond)                                                     \
    do {                                                                       \
      if (!(cond)) {                                                           \
        std::fprintf(stderr, "[t81] ASSERT FAILED: %s (%s:%d)\n",              \
                     #cond, __FILE__, __LINE__);                               \
        std::abort();                                                          \
      }                                                                        \
    } while (0)

  #define T81_ASSERT_MSG(cond, msg)                                            \
    do {                                                                       \
      if (!(cond)) {                                                           \
        std::fprintf(stderr, "[t81] ASSERT FAILED: %s | %s (%s:%d)\n",         \
                     #cond, (msg), __FILE__, __LINE__);                        \
        std::abort();                                                          \
      }                                                                        \
    } while (0)
#else
  #define T81_ASSERT(cond)       do { (void)sizeof(cond); } while (0)
  #define T81_ASSERT_MSG(c, msg) do { (void)sizeof(c); (void)sizeof(msg); } while (0)
#endif
