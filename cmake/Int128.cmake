include_guard(GLOBAL)

# Probe the selected compiler, standard, configuration and warning policy.
# clang-cl can parse __int128 without supplying its signed division helpers.
if(T81_USE_CXX23)
  set(_t81_probe_standard 23)
else()
  set(_t81_probe_standard 20)
endif()
if(MSVC)
  set(_t81_probe_options /W4 /WX /DNDEBUG)
else()
  set(_t81_probe_options -Wall -Wextra -Wpedantic -DNDEBUG)
endif()
set(_t81_saved_try_compile_type "${CMAKE_TRY_COMPILE_TARGET_TYPE}")
set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
try_compile(T81_NATIVE_INT128_LINKS
  "${CMAKE_BINARY_DIR}/native-int128-probe"
  SOURCES "${CMAKE_CURRENT_LIST_DIR}/probes/native_int128.cpp"
  CMAKE_FLAGS
    "-DCMAKE_CXX_STANDARD=${_t81_probe_standard}"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  COMPILE_DEFINITIONS ${_t81_probe_options}
  OUTPUT_VARIABLE _t81_probe_output)
if(_t81_saved_try_compile_type)
  set(CMAKE_TRY_COMPILE_TARGET_TYPE "${_t81_saved_try_compile_type}")
else()
  unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
endif()
file(WRITE "${CMAKE_BINARY_DIR}/native-int128-probe.log"
  "Compiler: ${CMAKE_CXX_COMPILER}\nCompiler ID/version: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\nTarget: ${CMAKE_CXX_COMPILER_TARGET}\nConfiguration: ${CMAKE_BUILD_TYPE}\nC++ standard: ${_t81_probe_standard}\nNative signed int128 compile/link: ${T81_NATIVE_INT128_LINKS}\n${_t81_probe_output}\n")
if(T81_NATIVE_INT128_LINKS)
  add_compile_definitions(T81_HAS_NATIVE_INT128=1)
  message(STATUS "T81 int128 backend: native (signed arithmetic compile/link probe passed)")
else()
  add_compile_definitions(T81_HAS_NATIVE_INT128=0)
  message(STATUS "T81 int128 backend: portable 128-bit (native compile/link probe failed)")
endif()
message(STATUS "T81 int128 capability evidence: ${CMAKE_BINARY_DIR}/native-int128-probe.log")
