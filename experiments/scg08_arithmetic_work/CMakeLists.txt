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

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME t81_exp_scg08_01_schema_validation
    COMMAND ${Python3_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/scripts/ci/validate_scg08_gate1_evidence.py
      ${CMAKE_CURRENT_SOURCE_DIR}/gate1_evidence_record.json
      ${CMAKE_CURRENT_SOURCE_DIR}/evidence_schema.json
  )
  set_tests_properties(t81_exp_scg08_01_schema_validation PROPERTIES TIMEOUT 30)
endif()
