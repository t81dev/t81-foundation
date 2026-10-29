# Also support a narrow standalone build without configuring production T81.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.16)
  project(t81_s02_scientific_integrity LANGUAGES CXX)
  option(T81_BUILD_TESTS "Build S02 integrity controls" ON)
  enable_testing()
endif()
add_executable(t81_s02_stochastic_topology main.cpp)
target_compile_features(t81_s02_stochastic_topology PRIVATE cxx_std_17)
target_include_directories(t81_s02_stochastic_topology PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(T81_BUILD_TESTS)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_executable(t81_s02_integrity_controls test_controls.cpp)
  target_compile_features(t81_s02_integrity_controls PRIVATE cxx_std_17)
  add_executable(t81_s02_regression_probe test_probe.cpp)
  target_compile_features(t81_s02_regression_probe PRIVATE cxx_std_17)
  add_test(NAME t81_s02_integrity_controls COMMAND t81_s02_integrity_controls)
  add_test(NAME t81_s02_stochastic_topology_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_oracle.py
      --runner $<TARGET_FILE:t81_s02_stochastic_topology>)
  add_test(NAME t81_s02_sampling_controls
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_sampling.py)
  add_test(NAME t81_s02_five_findings
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_integrity.py
      --probe $<TARGET_FILE:t81_s02_regression_probe>)
  add_test(NAME t81_s02_reporting_controls
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_reporting.py)
  add_test(NAME t81_s02_report_reproducibility
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/analyze_results.py --check)
  set_tests_properties(t81_s02_stochastic_topology_test PROPERTIES TIMEOUT 120)
endif()
