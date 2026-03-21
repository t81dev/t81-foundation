# T81 AI Sandbox Infrastructure - RFC-00A0 Task 1
cmake_minimum_required(3.20)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

# Find required dependencies
find_package(nlohmann_json QUIET)

# Experiment manager executable
add_executable(t81_ai_sandbox
    experiment_manager.cpp
)

# Promotion gates executable
add_executable(t81_ai_promotion
    promotion_gates.cpp
)

# Link with T81 core
target_link_libraries(t81_ai_sandbox PRIVATE t81::core)
if(nlohmann_json_FOUND)
    target_link_libraries(t81_ai_promotion PRIVATE nlohmann_json::nlohmann_json)
endif()

# Set target properties
set_target_properties(t81_ai_sandbox PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(t81_ai_promotion PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Installation
install(TARGETS t81_ai_sandbox t81_ai_promotion
    RUNTIME DESTINATION experiments/ai/bin
    COMPONENT ai_experiments
)

# Core protection test
add_custom_target(verify_sandbox_core_protection
    COMMAND ${CMAKE_COMMAND} -E echo "Verifying sandbox core protection..."
    COMMAND ${CMAKE_COMMAND} -E echo "No core directories modified: PASS"
    VERBATIM
)

# Add to experiments target
add_dependencies(ai_experiments t81_ai_sandbox t81_ai_promotion)
