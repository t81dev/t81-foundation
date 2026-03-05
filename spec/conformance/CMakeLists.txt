# spec/conformance/CMakeLists.txt
# RFC-0027 SE-M1: spec_conformance_all target
#
# Activation prerequisite: T81Lang compiler support for:
#   - @axion_verify annotation (required on every spec program's main())
#   - @pure annotation
#   - @tier(N) annotation
#   - @expect_fault(F) annotation (for negative conformance programs; RFC-0027 §5)
#
# Current status: SE-M1 scaffold.
# The add_custom_target below is a documentation stub.
# Replace with the add_test block in SE-M6 when the compiler is ready.
#
# --- SE-M6 activation (uncomment when compiler is ready) ---
#
# file(GLOB_RECURSE T81_SPEC_PROGRAMS
#     "${CMAKE_CURRENT_SOURCE_DIR}/**/*.t81"
# )
#
# foreach(prog ${T81_SPEC_PROGRAMS})
#     get_filename_component(prog_name "${prog}" NAME_WE)
#     get_filename_component(prog_dir  "${prog}" DIRECTORY)
#     file(RELATIVE_PATH prog_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${prog}")
#     string(REPLACE "/" "_" test_name "${prog_rel}")
#     string(REPLACE ".t81" "" test_name "${test_name}")
#     add_test(
#         NAME    "spec_conformance_${test_name}"
#         COMMAND t81 run "${prog}"
#     )
#     set_tests_properties("spec_conformance_${test_name}" PROPERTIES
#         LABELS "spec_conformance"
#     )
# endforeach()
#
# add_custom_target(spec_conformance_all
#     COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}"
#             -L spec_conformance --output-on-failure
#     COMMENT "Running spec conformance suite (RFC-0027)"
#     DEPENDS t81
# )
#
# --- end SE-M6 block ---

add_custom_target(spec_conformance_all
    COMMENT "Spec conformance suite stub — corpus authored, ctest activation pending missing @axion_verify/feature support"
)
