add_library(t81_vm STATIC
  ${PROJECT_SOURCE_DIR}/vm/vm.cpp
  ${PROJECT_SOURCE_DIR}/vm/tier_limits.cpp
  ${PROJECT_SOURCE_DIR}/vm/value_ops.cpp
  ${PROJECT_SOURCE_DIR}/vm/memory_segments.cpp
  ${PROJECT_SOURCE_DIR}/vm/policy_trace_bridge.cpp
  ${PROJECT_SOURCE_DIR}/vm/runtime_state_helpers.cpp
  ${PROJECT_SOURCE_DIR}/vm/gc_helpers.cpp
  ${PROJECT_SOURCE_DIR}/vm/tensor_helpers.cpp
  ${PROJECT_SOURCE_DIR}/vm/ffi_dispatcher.cpp
  # RFC-0032 Phase 4 -- promoted AI VM backend adapter (C-02)
  ${PROJECT_SOURCE_DIR}/vm/ai_backend/backend_adapter.cpp
  ${PROJECT_SOURCE_DIR}/vm/decode_state.cpp
  ${PROJECT_SOURCE_DIR}/ffi/ffi_dispatcher.cpp
)
t81_preserve_root_output(t81_vm)
target_include_directories(t81_vm PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_jit OBJECT
  ${PROJECT_SOURCE_DIR}/vm/jit/jit_compiler.cpp
  ${PROJECT_SOURCE_DIR}/vm/jit/jit_trace_cache.cpp   # RFC-0028 §4: CanonFS-backed trace cache
)
target_compile_features(t81_jit PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_jit PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

target_sources(t81_vm PRIVATE $<TARGET_OBJECTS:t81_jit>)
target_link_libraries(t81_vm PUBLIC t81_core t81_isa t81_axion t81_ffi t81_experimental)

if(T81_ENABLE_DPE)
  message(STATUS "[t81] T81_ENABLE_DPE=ON -- building DPE task graph (RFC-DPE-0002 through RFC-DPE-0009)")

  add_library(t81_dpe STATIC
    ${PROJECT_SOURCE_DIR}/vm/dpe/task_graph.cpp
    ${PROJECT_SOURCE_DIR}/vm/dpe/task_runner.cpp
    ${PROJECT_SOURCE_DIR}/vm/dpe/epoch_commit.cpp
    ${PROJECT_SOURCE_DIR}/vm/dpe/thread_pool.cpp
  )
  t81_preserve_root_output(t81_dpe)
  target_include_directories(t81_dpe PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  )
  target_link_libraries(t81_dpe PUBLIC t81_core t81_vm)
  target_compile_features(t81_dpe PUBLIC ${T81_CXX_STD_FEATURE})

  if(T81_BUILD_TESTS)
    add_executable(t81_dpe_task_graph_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/task_graph_test.cpp)
    t81_preserve_root_output(t81_dpe_task_graph_test)
    target_link_libraries(t81_dpe_task_graph_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_task_graph_test COMMAND t81_dpe_task_graph_test)

    add_executable(t81_dpe_task_runner_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/task_runner_test.cpp)
    t81_preserve_root_output(t81_dpe_task_runner_test)
    target_link_libraries(t81_dpe_task_runner_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_task_runner_test COMMAND t81_dpe_task_runner_test)

    add_executable(t81_dpe_epoch_commit_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/epoch_commit_test.cpp)
    t81_preserve_root_output(t81_dpe_epoch_commit_test)
    target_link_libraries(t81_dpe_epoch_commit_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_epoch_commit_test COMMAND t81_dpe_epoch_commit_test)

    add_executable(t81_dpe_epoch_dag_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/epoch_dag_test.cpp)
    t81_preserve_root_output(t81_dpe_epoch_dag_test)
    target_link_libraries(t81_dpe_epoch_dag_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_epoch_dag_test COMMAND t81_dpe_epoch_dag_test)

    add_executable(t81_dpe_epoch_parallel_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/epoch_parallel_test.cpp)
    t81_preserve_root_output(t81_dpe_epoch_parallel_test)
    target_link_libraries(t81_dpe_epoch_parallel_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_epoch_parallel_test COMMAND t81_dpe_epoch_parallel_test)

    add_executable(t81_dpe_thread_pool_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/thread_pool_test.cpp)
    t81_preserve_root_output(t81_dpe_thread_pool_test)
    target_link_libraries(t81_dpe_thread_pool_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_thread_pool_test COMMAND t81_dpe_thread_pool_test)

    add_executable(t81_dpe_float_reduction_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/dpe_float_reduction_test.cpp)
    t81_preserve_root_output(t81_dpe_float_reduction_test)
    target_link_libraries(t81_dpe_float_reduction_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_float_reduction_test COMMAND t81_dpe_float_reduction_test)

    add_executable(t81_dpe_include_surface_test
      ${PROJECT_SOURCE_DIR}/tests/cpp/dpe/include_surface_test.cpp)
    t81_preserve_root_output(t81_dpe_include_surface_test)
    target_link_libraries(t81_dpe_include_surface_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_include_surface_test COMMAND t81_dpe_include_surface_test)
  endif()
endif()
