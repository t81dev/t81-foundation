# =============================================================================
# Examples (extracted from root CMakeLists.txt in Wave 3)
# =============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_executable(t81_demo ${CMAKE_SOURCE_DIR}/examples/compiler-and-ffi/demo.cpp)
target_link_libraries(t81_demo PRIVATE t81_core)

add_executable(t81_tensor_ops ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/tensor_ops.cpp)
target_link_libraries(t81_tensor_ops PRIVATE t81_core)

add_executable(t81_ir_roundtrip ${CMAKE_SOURCE_DIR}/examples/compiler-and-ffi/ir_roundtrip.cpp)
target_link_libraries(t81_ir_roundtrip PRIVATE t81_core)

add_executable(t81_make_demo_model ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_demo_model.cpp)
target_link_libraries(t81_make_demo_model PRIVATE t81_tool_model t81_core)

add_executable(t81_make_demo_model_gov ${CMAKE_SOURCE_DIR}/examples/governance/governance-demo/make_demo_model_gov.cpp)
target_link_libraries(t81_make_demo_model_gov PRIVATE t81_tool_model t81_core)

add_executable(t81_make_guarded_llama_demo
               ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp)
target_link_libraries(t81_make_guarded_llama_demo PRIVATE t81_tool_model t81_core)

add_executable(t81_make_answer_fixed_demo
               ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp)
target_link_libraries(t81_make_answer_fixed_demo PRIVATE t81_tool_model t81_core)
add_executable(t81_make_classify_fixed_demo
               ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp)
target_link_libraries(t81_make_classify_fixed_demo PRIVATE t81_tool_model t81_core)
add_executable(t81_make_route_fixed_demo
               ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp)
target_link_libraries(t81_make_route_fixed_demo PRIVATE t81_tool_model t81_core)
add_executable(t81_make_assess_fixed_demo
               ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp)
target_link_libraries(t81_make_assess_fixed_demo PRIVATE t81_tool_model t81_core)

add_executable(t81_make_degraded_llama_demo
               ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp)
target_link_libraries(t81_make_degraded_llama_demo PRIVATE t81_tool_model t81_core)

add_executable(t81_make_demo_safetensors ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_demo_safetensors.cpp)
target_link_libraries(t81_make_demo_safetensors PRIVATE t81_core)

add_executable(t81_make_demo_float_safetensors
               ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_demo_float_safetensors.cpp)
target_link_libraries(t81_make_demo_float_safetensors PRIVATE t81_core)

add_executable(t81_make_demo_gguf ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/model-load-canonfs/make_demo_gguf.cpp)
target_link_libraries(t81_make_demo_gguf PRIVATE t81_core)

add_executable(axion_demo ${CMAKE_SOURCE_DIR}/examples/governance/axion_demo.cpp)
target_link_libraries(axion_demo PRIVATE t81_core t81_axion)

add_executable(axion_guard_trace ${CMAKE_SOURCE_DIR}/examples/governance/axion_guard_trace.cpp)
target_link_libraries(axion_guard_trace PRIVATE t81_tool_cli t81_vm)

add_executable(axion_policy_trace ${CMAKE_SOURCE_DIR}/examples/governance/axion_policy_trace.cpp)
target_link_libraries(axion_policy_trace PRIVATE t81_tool_cli t81_vm)

add_executable(axion_policy_runner ${CMAKE_SOURCE_DIR}/examples/governance/axion_policy_runner.cpp)
target_link_libraries(axion_policy_runner PRIVATE t81_tool_cli t81_vm)

add_executable(llama32_demo ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/ai-integration/llama32_demo.cpp)
target_link_libraries(llama32_demo PRIVATE t81_tool_cli t81_vm)

if(T81_ENABLE_LLAMA_CPP)
add_executable(llama_cpp_governed_demo ${CMAKE_SOURCE_DIR}/examples/ai-and-inference/ai-integration/llama_cpp_governed_demo.cpp)
target_link_libraries(llama_cpp_governed_demo PRIVATE t81_llama_adapter)
endif()

add_executable(canonfs_trace_demo ${CMAKE_SOURCE_DIR}/examples/storage-and-canonfs/canonfs_trace_demo.cpp)
target_link_libraries(canonfs_trace_demo PRIVATE t81_core t81_axion)

add_executable(complex_demo ${CMAKE_SOURCE_DIR}/examples/compiler-and-ffi/complex_demo.cpp)
target_link_libraries(complex_demo PRIVATE t81_core asio)
if(WIN32)
  target_link_libraries(complex_demo PRIVATE ws2_32 mswsock)
endif()

add_executable(t81_bitnet_2b_dispatch_test
  ${CMAKE_SOURCE_DIR}/examples/research/heterogeneous-dualrail/verify_bitnet_2b_dispatch.cpp)
target_include_directories(t81_bitnet_2b_dispatch_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(t81_bitnet_2b_dispatch_test PRIVATE t81_regime_dispatch t81_provider t81_crypto t81_core)
if(T81_BUILD_TESTS)
  add_test(NAME t81_bitnet_2b_dispatch_test COMMAND t81_bitnet_2b_dispatch_test)
endif()

