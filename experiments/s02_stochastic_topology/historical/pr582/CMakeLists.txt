add_executable(t81_s02_stochastic_topology main.cpp)
target_include_directories(t81_s02_stochastic_topology PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(T81_BUILD_TESTS)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME t81_s02_stochastic_topology_test
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/test_oracle.py
      --runner $<TARGET_FILE:t81_s02_stochastic_topology>)
  set_tests_properties(t81_s02_stochastic_topology_test PROPERTIES TIMEOUT 120)
endif()
