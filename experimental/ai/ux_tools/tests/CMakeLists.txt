# T81 AI CLI Tests - RFC-00A7 Task 10
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Test executable
add_executable(test_ai_cli
    test_ai_cli.cpp
)

# Link with required libraries
target_link_libraries(test_ai_cli PRIVATE 
    t81::core
)

# Set target properties
set_target_properties(test_ai_cli PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Enable testing
enable_testing()

# Register tests
add_test(NAME AICLI 
         COMMAND test_ai_cli
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Add to experiments target
add_dependencies(ai_experiments test_ai_cli)
