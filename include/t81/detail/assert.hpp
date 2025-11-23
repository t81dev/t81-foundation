#pragma once
#include <cstdio>
#include <cstdlib>
#include "t81/config.hpp"

// Lightweight internal assertions used across headers.
// Enabled when T81_ENABLE_ASSERTS != 0 (see config.hpp).

#if T81_ENABLE_ASSERTS

  #define T81_ASSERT_IMPL_(cond, msg)                                            \
    do {                                                                         \
      if (!(cond)) {                                                             \
        std::fprintf(stderr,                                                     \
          "[T81 ASSERT] %s:%d: %s\n", __FILE__, __LINE__, (msg));                \
        std::abort();                                                            \
      }                                                                          \
    } while (0)

  #define T81_ASSERT(cond)        T81_ASSERT_IMPL_((cond), #cond)
  #define T81_ASSERT_MSG(cond,m)  T81_ASSERT_IMPL_((cond), (m))

#else

  #define T81_ASSERT(cond)        do { (void)sizeof(cond); } while (0)
  #define T81_ASSERT_MSG(c,m)     do { (void)sizeof(c); (void)sizeof(m); } while (0)

#endif
