#pragma once
// Legacy shim — prefer including <t81/axion/api.hpp> and using t81::axion::Signal.
#include "t81/axion/api.hpp"

#if !defined(T81_SUPPRESS_DEPRECATION)
  #if defined(_MSC_VER)
    #pragma message("axion_signal.hpp is a legacy shim; use <t81/axion/api.hpp> (Signal) instead.")
  #else
    #warning "axion_signal.hpp is a legacy shim; use <t81/axion/api.hpp> (Signal) instead."
  #endif
#endif
