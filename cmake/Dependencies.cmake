include_guard(GLOBAL)

if(T81_ENABLE_LLAMA_CPP)
  set(T81_LLAMA_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp")
  if(NOT EXISTS "${T81_LLAMA_CPP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
      "T81_ENABLE_LLAMA_CPP=ON but llama.cpp was not found at ${T81_LLAMA_CPP_DIR}. "
      "Clone https://github.com/ggml-org/llama.cpp.git into third_party/llama.cpp.")
  endif()

  # Keep the embedded dependency minimal and deterministic: library-only build.
  set(LLAMA_BUILD_COMMON OFF CACHE BOOL "llama.cpp common helpers" FORCE)
  set(LLAMA_BUILD_TESTS OFF CACHE BOOL "llama.cpp tests" FORCE)
  set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "llama.cpp tools" FORCE)
  set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "llama.cpp examples" FORCE)
  set(LLAMA_BUILD_SERVER OFF CACHE BOOL "llama.cpp server" FORCE)
  add_subdirectory("${T81_LLAMA_CPP_DIR}" "${CMAKE_BINARY_DIR}/third_party/llama.cpp" EXCLUDE_FROM_ALL)
  message(STATUS "T81_ENABLE_LLAMA_CPP=ON (using ${T81_LLAMA_CPP_DIR})")
endif()

# FTXUI -- TUI frontends (RFC-0033)
if(T81_BUILD_TUI)
  include(FetchContent)
  FetchContent_Declare(
    ftxui
    URL https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v5.0.0.tar.gz
  )
  set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(ftxui)
  message(STATUS "T81_BUILD_TUI=ON (FTXUI v5.0.0)")
endif()
