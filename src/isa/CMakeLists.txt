# t81_math_quantization -- deterministic ternary codec (RFC-0032 Phase 1, C-01).
# Integer-only; no float, no timing, no external deps. Safe for deterministic core.
add_library(t81_math_quantization STATIC
  ${PROJECT_SOURCE_DIR}/isa/math/quantization/ternary_codec.cpp
)
t81_preserve_root_output(t81_math_quantization)
target_compile_features(t81_math_quantization PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_math_quantization PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(t81_math_quantization PUBLIC t81_core)

add_library(t81_isa STATIC
  ${PROJECT_SOURCE_DIR}/isa/pretty_printer.cpp
  ${PROJECT_SOURCE_DIR}/isa/binary_emitter.cpp
  ${PROJECT_SOURCE_DIR}/isa/binary_io.cpp
  ${PROJECT_SOURCE_DIR}/isa/base81_view.cpp
)
t81_preserve_root_output(t81_isa)
target_link_libraries(t81_isa PUBLIC t81_core)
target_include_directories(t81_isa PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
