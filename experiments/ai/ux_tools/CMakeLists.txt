# T81 AI CLI Tools - Minimal Implementation
cmake_minimum_required(VERSION 3.16)

# Only build if AI experiments are enabled
if(NOT T81_ENABLE_AI_EXPERIMENTS)
    return()
endif()

message(STATUS "Building minimal AI CLI")

# Minimal AI CLI executable
add_executable(t81_ai
    t81_ai_minimal.cpp
)

# Set target properties (no external dependencies)
set_target_properties(t81_ai PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Installation
install(TARGETS t81_ai
    RUNTIME DESTINATION experiments/ai/bin
    COMPONENT ai_experiments
)

message(STATUS "t81_ai minimal executable configured")
