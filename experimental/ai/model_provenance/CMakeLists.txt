# T81 Model Provenance System - RFC-00A3 Task 5
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Find required dependencies
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json QUIET)

# Model manager executable
add_executable(t81_ai_model
    model_manager.cpp
)

# Link with required libraries
target_link_libraries(t81_ai_model PRIVATE 
    t81::core
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(nlohmann_json_FOUND)
    target_link_libraries(t81_ai_model PRIVATE nlohmann_json::nlohmann_json)
endif()

# Set target properties
set_target_properties(t81_ai_model PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Installation
install(TARGETS t81_ai_model
    RUNTIME DESTINATION experiments/ai/bin
    COMPONENT ai_experiments
)

# CanonFS integration
add_custom_target(init_canonfs_model_storage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/canonfs/models
    COMMAND ${CMAKE_COMMAND} -E echo "CanonFS model storage initialized"
    VERBATIM
)

# Test suite
add_subdirectory(tests)

# Add to experiments target
add_dependencies(ai_experiments t81_ai_model init_canonfs_model_storage)
