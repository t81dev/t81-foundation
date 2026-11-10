cmake_minimum_required(VERSION 3.16)

add_executable(t81_exp_scg08_01_test
  test_exp_scg08_01.cpp
)

target_include_directories(t81_exp_scg08_01_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(t81_exp_scg08_01_test PRIVATE
  t81_core
)

if(T81_BUILD_TESTS)
  add_test(NAME t81_exp_scg08_01_test
    COMMAND $<TARGET_FILE:t81_exp_scg08_01_test>
  )
  set_tests_properties(t81_exp_scg08_01_test PROPERTIES TIMEOUT 60)
endif()
