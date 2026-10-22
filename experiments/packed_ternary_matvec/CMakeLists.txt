cmake_minimum_required(VERSION 3.16)

include(CheckCXXCompilerFlag)

add_executable(t81_packed_ternary_matvec
  main.cpp
  matvec_experiment.hpp
)

target_include_directories(t81_packed_ternary_matvec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

check_cxx_compiler_flag("-mavx2" COMPILER_HAS_AVX2)
check_cxx_compiler_flag("-mfma" COMPILER_HAS_FMA)
if(COMPILER_HAS_AVX2 AND COMPILER_HAS_FMA)
  target_compile_options(t81_packed_ternary_matvec PRIVATE -mavx2 -mfma)
endif()

if(T81_BUILD_TESTS)
  add_test(NAME t81_packed_ternary_matvec_oracle_test
    COMMAND $<TARGET_FILE:t81_packed_ternary_matvec> --json-out ${CMAKE_CURRENT_BINARY_DIR}/packed_ternary_matvec_results.json
  )
endif()
