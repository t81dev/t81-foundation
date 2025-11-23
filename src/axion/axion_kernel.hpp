#pragma once
// Legacy shim — prefer including <t81/axion/api.hpp> and using t81::axion types.
#include "t81/axion/api.hpp"

#if !defined(T81_SUPPRESS_DEPRECATION)
  #if defined(_MSC_VER)
    #pragma message("axion_kernel.hpp is a legacy shim; include <t81/axion/api.hpp> instead.")
  #else
    #warning "axion_kernel.hpp is a legacy shim; include <t81/axion/api.hpp> instead."
  #endif
#endif
