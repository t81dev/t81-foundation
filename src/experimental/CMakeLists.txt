add_library(t81_experimental STATIC
  ${PROJECT_SOURCE_DIR}/experimental/hanoi/error.cpp
  ${PROJECT_SOURCE_DIR}/experimental/hanoi/in_memory_kernel.cpp
  ${PROJECT_SOURCE_DIR}/experimental/tiers/cog/promotion.cpp
  ${PROJECT_SOURCE_DIR}/experimental/tiers/cog/tier2/reflective.cpp
  ${PROJECT_SOURCE_DIR}/experimental/tiers/cog/tier3/recursive.cpp
  ${PROJECT_SOURCE_DIR}/experimental/tiers/cog/tier4/tier4_loop.cpp
  ${PROJECT_SOURCE_DIR}/experimental/distributed/distributed.cpp
  ${PROJECT_SOURCE_DIR}/experimental/tiers/cog/tier5/infinite.cpp
)
t81_preserve_root_output(t81_experimental)
target_link_libraries(t81_experimental PUBLIC t81_core)
target_compile_features(t81_experimental PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_experimental PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

function(t81_add_experimental_tool_targets)
  if(T81_ENABLE_AI_EXPERIMENTS)
    add_executable(t81_ai
      ${PROJECT_SOURCE_DIR}/tools/cli/ai/t81_ai_cli.cpp
      ${PROJECT_SOURCE_DIR}/tools/cli/ai/ai_cli_shared.cpp
    )
    t81_preserve_root_output(t81_ai)
    target_link_libraries(t81_ai PRIVATE
      t81_decode
      t81_tool_cli
      t81_vm
      t81_axion
      t81_math_quantization
      t81_core
    )
    set(AI_EXP_DIR "${CMAKE_BINARY_DIR}/experiments")
    set_target_properties(t81_ai PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${AI_EXP_DIR}/ai/ux_tools")
  endif()
endfunction()
