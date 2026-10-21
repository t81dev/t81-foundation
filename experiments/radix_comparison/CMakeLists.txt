cmake_minimum_required(VERSION 3.16)

add_executable(t81_radix_comparison
  main.cpp
  radix_comparison.hpp
)

target_include_directories(t81_radix_comparison PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(T81_BUILD_TESTS)
  add_test(NAME t81_radix_comparison_test
    COMMAND $<TARGET_FILE:t81_radix_comparison> --json-out ${CMAKE_CURRENT_BINARY_DIR}/radix_comparison_results.json
  )
endif()
