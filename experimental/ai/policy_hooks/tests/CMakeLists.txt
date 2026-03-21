# T81 Axion Policy Hooks Tests - RFC-00A6 Task 8
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Test executable
add_executable(test_policy_hooks
    test_policy_hooks.cpp
)

# Link with required libraries
target_link_libraries(test_policy_hooks PRIVATE 
    t81::core
)

# Set target properties
set_target_properties(test_policy_hooks PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Enable testing
enable_testing()

# Register tests
add_test(NAME PolicyHooks 
         COMMAND test_policy_hooks
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Add to experiments target
add_dependencies(ai_experiments test_policy_hooks)
