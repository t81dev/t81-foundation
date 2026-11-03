// Include the actual test so reverting its Release failure handling breaks this guard.
#ifndef NDEBUG
#error "Release correctness guards must compile with NDEBUG defined"
#endif

#define main original_correctness_test_main
#if defined(T81_RELEASE_GUARD_SHA3)
#include "test_sha3.cpp"
#elif defined(T81_RELEASE_GUARD_FLOAT)
#include "test_deterministic_float_boundaries.cpp"
#elif defined(T81_RELEASE_GUARD_MAP)
#include "test_map_determinism.cpp"
#else
#error "Select one correctness test"
#endif
#undef main

#include <cstdlib>

int main() {
#ifdef _MSC_VER
  _set_error_mode(_OUT_TO_STDERR);
#endif
#if defined(T81_RELEASE_GUARD_SHA3)
  run_test("", "deliberately incorrect digest", "release correctness guard");
#else
  assert(false && "release correctness guard");
#endif
  // Reaching this return means a critical correctness failure was ignored.
  return EXIT_SUCCESS;
}
