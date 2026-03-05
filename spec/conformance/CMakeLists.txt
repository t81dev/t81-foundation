# spec/conformance/CMakeLists.txt
# RFC-0027: executable conformance corpus wiring

file(GLOB_RECURSE T81_SPEC_PROGRAMS
    "${CMAKE_CURRENT_SOURCE_DIR}/**/*.t81"
)

foreach(prog ${T81_SPEC_PROGRAMS})
    file(RELATIVE_PATH prog_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${prog}")
    string(REPLACE "/" "_" test_name "${prog_rel}")
    string(REPLACE ".t81" "" test_name "${test_name}")
    add_test(
        NAME "spec_conformance_${test_name}"
        COMMAND $<TARGET_FILE:t81> code run "${prog}"
    )
    set_tests_properties("spec_conformance_${test_name}" PROPERTIES
        LABELS "spec_conformance"
    )
endforeach()

add_custom_target(spec_conformance_all
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}"
            -L spec_conformance --output-on-failure
    COMMENT "Running spec conformance suite (RFC-0027)"
    DEPENDS t81
)
