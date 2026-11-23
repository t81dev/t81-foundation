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
  add_test(NAME t81_exp_scg08_01_result_rejection
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/test_result_rejection.py
      $<TARGET_FILE:t81_exp_scg08_01_test>
  )
  set_tests_properties(t81_exp_scg08_01_result_rejection PROPERTIES TIMEOUT 150)

  add_test(NAME t81_exp_scg08_01_result_rejection_driver
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/test_result_rejection_driver.py
  )
  set_tests_properties(t81_exp_scg08_01_result_rejection_driver PROPERTIES TIMEOUT 30)

  add_test(NAME t81_exp_scg08_01_schema_validation
    COMMAND ${Python3_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/scripts/ci/validate_scg08_gate1_evidence.py
      ${CMAKE_CURRENT_SOURCE_DIR}/gate1_evidence_record.json
      ${CMAKE_CURRENT_SOURCE_DIR}/evidence_schema.json
  )
  set_tests_properties(t81_exp_scg08_01_schema_validation PROPERTIES TIMEOUT 30)

  add_test(NAME t81_exp_scg08_01_schema_negative_validation
    COMMAND ${Python3_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/scripts/ci/validate_scg08_gate1_evidence.py
      --test-negative
  )
  set_tests_properties(t81_exp_scg08_01_schema_negative_validation PROPERTIES TIMEOUT 30)

  # File/CLI rejection paths and optimized Python checks remain active in Release.
  add_test(NAME t81_exp_scg08_01_schema_json_files
    COMMAND ${Python3_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/scripts/ci/validate_scg08_gate1_evidence.py
      --test-json-files
  )
  foreach(mode negative json-files)
    string(REPLACE "-" "_" test_mode "${mode}")
    add_test(NAME t81_exp_scg08_01_schema_${test_mode}_optimized
      COMMAND ${Python3_EXECUTABLE} -O
        ${PROJECT_SOURCE_DIR}/scripts/ci/validate_scg08_gate1_evidence.py
        --test-${mode}
    )
    set_tests_properties(t81_exp_scg08_01_schema_${test_mode}_optimized
      PROPERTIES TIMEOUT 60)
  endforeach()
  set_tests_properties(t81_exp_scg08_01_schema_json_files PROPERTIES TIMEOUT 60)
endif()
