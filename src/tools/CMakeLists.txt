add_library(t81_tool_model STATIC
  ${PROJECT_SOURCE_DIR}/tools/model/gguf_import_bridge.cpp
  ${PROJECT_SOURCE_DIR}/tools/model/weights.cpp
  ${PROJECT_SOURCE_DIR}/tools/model/weights_t81w_emitter.cpp
)
t81_preserve_root_output(t81_tool_model)
target_link_libraries(t81_tool_model PUBLIC t81_core)
target_include_directories(t81_tool_model PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(T81_ENABLE_LLAMA_CPP)
  target_link_libraries(t81_tool_model PRIVATE $<BUILD_INTERFACE:llama>)
  target_compile_definitions(t81_tool_model PRIVATE T81_HAS_LLAMA_CPP=1)
endif()

if(T81_ENABLE_LLAMA_CPP)
  add_library(t81_llama_adapter STATIC
    ${PROJECT_SOURCE_DIR}/tools/model/llama_cpp_adapter.cpp
  )
  t81_preserve_root_output(t81_llama_adapter)
  target_compile_features(t81_llama_adapter PUBLIC ${T81_CXX_STD_FEATURE})
  target_include_directories(t81_llama_adapter PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_link_libraries(t81_llama_adapter PUBLIC t81_core t81_axion llama)
  target_compile_definitions(t81_llama_adapter PUBLIC T81_HAS_LLAMA_CPP=1)
endif()

add_library(t81_tool_cli STATIC
  ${PROJECT_SOURCE_DIR}/tools/cli/ai/ai_cli_shared.cpp
  ${PROJECT_SOURCE_DIR}/tools/cli/ai/t81_ai_cli.cpp
  # ${PROJECT_SOURCE_DIR}/tools/cli/ai/t81_ai_controlled.cpp  # Skip problematic file
  ${PROJECT_SOURCE_DIR}/tools/cli/core/artifact_family.cpp
  ${PROJECT_SOURCE_DIR}/tools/cli/core/canonize_tensor.cpp
  ${PROJECT_SOURCE_DIR}/tools/cli/core/debugger.cpp
  ${PROJECT_SOURCE_DIR}/tools/cli/core/driver.cpp
  # ${PROJECT_SOURCE_DIR}/tools/cli/core/driver_refactored.cpp  # Skip problematic file
  ${PROJECT_SOURCE_DIR}/tools/cli/core/main.cpp
  # ${PROJECT_SOURCE_DIR}/tools/cli/canonfs_tools/canonfs_performance.cpp  # Skip problematic file
  ${PROJECT_SOURCE_DIR}/tools/cli/canonfs_tools/canonfs_performance_standalone.cpp
  # ${PROJECT_SOURCE_DIR}/tools/cli/canonfs_tools/canonfs_auto_optimizer.cpp  # Skip problematic file
  ${PROJECT_SOURCE_DIR}/tools/diagnostics/ternary_codec_metrics.cpp)

# Add standalone performance tool as executable
add_executable(canonfs_performance_standalone
  ${PROJECT_SOURCE_DIR}/tools/cli/canonfs_tools/canonfs_performance_standalone.cpp)

target_link_libraries(canonfs_performance_standalone PRIVATE t81_core)

# Experimental demos moved to experimental/ directory
# See: experimental/ai_integration_demos/

# Skip problematic controlled AI backend for now
# list(APPEND TOOL_SOURCES
#   ai/t81_ai_controlled.cpp
# )

t81_preserve_root_output(t81_tool_cli)
target_link_libraries(t81_tool_cli PUBLIC t81_lang_frontend t81_isa t81_vm t81_tool_model)
target_link_libraries(t81_tool_cli PUBLIC t81_llvm t81_mlir t81_c_frontend t81_rust_frontend t81_python_frontend)
target_include_directories(t81_tool_cli PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(t81_tool_cli PRIVATE
  ${PROJECT_SOURCE_DIR}
)

add_library(t81_decode STATIC
  ${PROJECT_SOURCE_DIR}/vm/decoder.cpp
)
t81_preserve_root_output(t81_decode)
target_link_libraries(t81_decode PUBLIC t81_tool_cli t81_vm t81_tool_model)
target_include_directories(t81_decode PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(T81_BUILD_TUI)
  add_library(t81_tui STATIC
    ${PROJECT_SOURCE_DIR}/tools/tui/common.cpp
    ${PROJECT_SOURCE_DIR}/tools/tui/studio.cpp
    ${PROJECT_SOURCE_DIR}/tools/tui/agent.cpp
  )
  t81_preserve_root_output(t81_tui)
  target_link_libraries(t81_tui PUBLIC t81_tool_cli ftxui::screen ftxui::dom ftxui::component)
  target_include_directories(t81_tui PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_include_directories(t81_tui PRIVATE ${PROJECT_SOURCE_DIR})
  target_compile_features(t81_tui PUBLIC ${T81_CXX_STD_FEATURE})

  if(T81_BUILD_TESTS)
    add_executable(tui_snapshot_test ${PROJECT_SOURCE_DIR}/tests/cpp/tui_snapshot_test.cpp)
    t81_preserve_root_output(tui_snapshot_test)
    target_link_libraries(tui_snapshot_test PRIVATE t81_tui ftxui::screen ftxui::dom)
    target_include_directories(tui_snapshot_test PRIVATE
      ${PROJECT_SOURCE_DIR}
      ${PROJECT_SOURCE_DIR}/tests/cpp
    )
    target_compile_features(tui_snapshot_test PRIVATE ${T81_CXX_STD_FEATURE})
    add_test(NAME tui_snapshot_test COMMAND tui_snapshot_test)
  endif()
endif()

add_executable(t81
  ${PROJECT_SOURCE_DIR}/tools/cli/core/main.cpp
  ${PROJECT_SOURCE_DIR}/tools/cli/ai/ai_cli_shared.cpp
)
t81_preserve_root_output(t81)
target_link_libraries(t81 PRIVATE t81_decode t81_lang_frontend t81_isa t81_vm t81_tool_cli t81_axion t81_math_quantization t81_core)
target_include_directories(t81 PRIVATE
  ${PROJECT_SOURCE_DIR}
)
if(T81_ENABLE_LLAMA_CPP)
  target_link_libraries(t81 PRIVATE t81_llama_adapter)
  target_compile_definitions(t81 PRIVATE T81_HAS_LLAMA_CPP=1)
endif()
if(T81_BUILD_TUI)
  target_link_libraries(t81 PRIVATE t81_tui)
  target_compile_definitions(t81 PRIVATE T81_HAS_TUI=1)
endif()

# RFC-0032 Phase 5 (C-07) -- promoted AI CLI tool (no sleep_for / no llama.cpp)
add_executable(t81_ai_cli
  ${PROJECT_SOURCE_DIR}/tools/cli/ai/t81_ai_cli.cpp
  ${PROJECT_SOURCE_DIR}/tools/cli/ai/ai_cli_shared.cpp
)
t81_preserve_root_output(t81_ai_cli)
target_link_libraries(t81_ai_cli PRIVATE t81_decode t81_tool_cli t81_vm t81_axion t81_math_quantization t81_core)

t81_add_experimental_tool_targets()
