# T81 Ternary Quantization Codec Tests - RFC-00A4 Task 6
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Test executable
add_executable(test_ternary_codec
    test_ternary_codec.cpp
)

# Link with required libraries
target_link_libraries(test_ternary_codec PRIVATE 
    t81::core
)

# Set target properties
set_target_properties(test_ternary_codec PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Enable testing
enable_testing()

# Register tests
add_test(NAME TernaryCodec 
         COMMAND test_ternary_codec
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Add to experiments target
add_dependencies(ai_experiments test_ternary_codec)
