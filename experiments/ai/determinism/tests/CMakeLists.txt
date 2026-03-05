# T81 Deterministic Evidence Framework Tests - RFC-00A1 Task 3
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Find required dependencies
find_package(nlohmann_json QUIET)

# Test executable
add_executable(test_determinism_framework
    test_determinism_framework.cpp
)

# Link with required libraries
target_link_libraries(test_determinism_framework PRIVATE 
    t81::core
)

if(nlohmann_json_FOUND)
    target_link_libraries(test_determinism_framework PRIVATE nlohmann_json::nlohmann_json)
endif()

# Set target properties
set_target_properties(test_determinism_framework PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Enable testing
enable_testing()

# Register tests
add_test(NAME DeterminismFramework 
         COMMAND test_determinism_framework
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Add to experiments target
add_dependencies(ai_experiments test_determinism_framework)
