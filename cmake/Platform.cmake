include_guard(GLOBAL)

# Detect Homebrew/non-system LLVM libc++ directory
# When using Homebrew LLVM on macOS, libc++ lives in <compiler-root>/lib/c++
# rather than the system library path. Static archives compiled with LLVM 21+
# reference std::__1::__hash_memory as an out-of-line symbol, so executables
# that link those archives must have the LLVM lib/c++ dir on their search path.
if(APPLE)
  get_filename_component(_T81_CXX_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(_T81_CXX_ROOT "${_T81_CXX_BIN_DIR}" DIRECTORY)
  if(EXISTS "${_T81_CXX_ROOT}/lib/c++/libc++.dylib")
    set(T81_LLVM_LIBCXX_DIR "${_T81_CXX_ROOT}/lib/c++")
    message(STATUS "[t81] Detected LLVM libc++ at ${T81_LLVM_LIBCXX_DIR}")
    # Apply globally: every executable in this build needs the LLVM lib/c++ dir
    # on its linker search path so std::__1::__hash_memory resolves correctly.
    add_link_options("-L${T81_LLVM_LIBCXX_DIR}")
  endif()
endif()

# CPU feature gating used for SIMD source/flags.
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" T81_SYSTEM_PROCESSOR_LOWER)
set(T81_IS_X86_64 OFF)
if(T81_SYSTEM_PROCESSOR_LOWER STREQUAL "x86_64" OR T81_SYSTEM_PROCESSOR_LOWER STREQUAL "amd64")
  set(T81_IS_X86_64 ON)
endif()
