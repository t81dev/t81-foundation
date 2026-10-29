cmake_minimum_required(VERSION 3.16)

add_executable(t81_h04_r02_adaptive
  main.cpp
  adaptive_experiment.hpp
)

target_compile_features(t81_h04_r02_adaptive PRIVATE cxx_std_17)

target_include_directories(t81_h04_r02_adaptive PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(T81_BUILD_TESTS)
  add_test(NAME t81_h04_r02_adaptive_test
    COMMAND $<TARGET_FILE:t81_h04_r02_adaptive> --json-out ${CMAKE_CURRENT_BINARY_DIR}/h04_r02_results.json
  )

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME t81_h04_r02_oracle_test
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/oracle.py
  )
  add_test(NAME t81_h04_r02_audit_regression_test
    COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tests/ci/h04_r02_audit_regression_test.py
      --bin-path $<TARGET_FILE:t81_h04_r02_adaptive>)
  set_tests_properties(t81_h04_r02_audit_regression_test PROPERTIES TIMEOUT 120)
endif()
