# T81 LLM Backend Adapter Tests - RFC-00A5 Task 7
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Test executable
add_executable(test_backend_adapter
    test_backend_adapter.cpp
)

# Link with required libraries
target_link_libraries(test_backend_adapter PRIVATE 
    t81::core
)

# Set target properties
set_target_properties(test_backend_adapter PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Enable testing
enable_testing()

# Register tests
add_test(NAME BackendAdapter 
         COMMAND test_backend_adapter
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Add to experiments target
add_dependencies(ai_experiments test_backend_adapter)
