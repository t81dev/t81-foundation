# T81 Model Provenance System Tests - RFC-00A3 Task 5
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Find required dependencies
find_package(nlohmann_json QUIET)

# Test executable
add_executable(test_model_provenance
    test_model_provenance.cpp
)

# Link with required libraries
target_link_libraries(test_model_provenance PRIVATE 
    t81::core
)

if(nlohmann_json_FOUND)
    target_link_libraries(test_model_provenance PRIVATE nlohmann_json::nlohmann_json)
endif()

# Set target properties
set_target_properties(test_model_provenance PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Enable testing
enable_testing()

# Register tests
add_test(NAME ModelProvenance 
         COMMAND test_model_provenance
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Add to experiments target
add_dependencies(ai_experiments test_model_provenance)
