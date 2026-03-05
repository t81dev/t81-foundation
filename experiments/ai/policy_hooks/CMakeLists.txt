# T81 Axion Policy Hooks - RFC-00A6 Task 8
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Find required dependencies
find_package(nlohmann_json QUIET)

# Policy hooks executable
add_executable(t81_ai_policy
    axion_hooks.cpp
)

# Link with required libraries
target_link_libraries(t81_ai_policy PRIVATE 
    t81::core
)

if(nlohmann_json_FOUND)
    target_link_libraries(t81_ai_policy PRIVATE nlohmann_json::nlohmann_json)
endif()

# Set target properties
set_target_properties(t81_ai_policy PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Installation
install(TARGETS t81_ai_policy
    RUNTIME DESTINATION experiments/ai/bin
    COMPONENT ai_experiments
)

# Test suite
add_subdirectory(tests)

# Add to experiments target
add_dependencies(ai_experiments t81_ai_policy)
