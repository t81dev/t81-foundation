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

# ARM64 Cross-compilation support when aarch64-linux-gnu-g++ is present
find_program(AARCH64_CXX_COMPILER aarch64-linux-gnu-g++)
find_program(QEMU_AARCH64_EXECUTABLE qemu-aarch64)

if(AARCH64_CXX_COMPILER AND QEMU_AARCH64_EXECUTABLE)
  add_custom_target(t81_packed_ternary_matvec_arm64 ALL
    COMMAND ${AARCH64_CXX_COMPILER} -O3 -march=armv8-a+simd
            -I${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/t81_packed_ternary_matvec_arm64
    COMMENT "Building ARM64 NEON candidate binary with aarch64-linux-gnu-g++"
    SOURCES main.cpp matvec_experiment.hpp
  )

  if(T81_BUILD_TESTS)
    add_test(NAME t81_packed_ternary_matvec_arm64_qemu_test
      COMMAND ${QEMU_AARCH64_EXECUTABLE} -L /usr/aarch64-linux-gnu
              ${CMAKE_CURRENT_BINARY_DIR}/t81_packed_ternary_matvec_arm64
              --repetitions 5
              --json-out ${CMAKE_CURRENT_BINARY_DIR}/packed_ternary_matvec_arm64_results.json
    )
  endif()
endif()
