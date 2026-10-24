# T81 Direct API CMakeLists.txt
# Implements high-performance direct API to bypass CLI overhead

add_library(t81_direct_api STATIC
    t81_direct_api.cpp
    ${PROJECT_SOURCE_DIR}/vm/ai_backend/controlled_ai_backend.cpp
)

target_link_libraries(t81_direct_api PUBLIC
    t81_core
    t81_axion
    t81_tracing
    t81_tool_model
    t81_tool_cli
    t81_vm
)

target_include_directories(t81_direct_api PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(t81_direct_api PUBLIC cxx_std_17)

# Add test executable
add_executable(test_real_direct_api
    ${PROJECT_SOURCE_DIR}/development_tests/test_real_direct_api.cpp
)

target_link_libraries(test_real_direct_api PRIVATE
    t81_direct_api
)

t81_preserve_root_output(test_real_direct_api)
