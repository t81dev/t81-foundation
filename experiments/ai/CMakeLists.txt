# T81 AI Experiments - Minimal Integration
cmake_minimum_required(VERSION 3.16)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

message(STATUS "Building T81 AI experiments")

# Create experiments target
add_custom_target(ai_experiments
    COMMENT "T81 AI experiments build target"
)

# Add only ux_tools for minimal implementation
add_subdirectory(ux_tools)
