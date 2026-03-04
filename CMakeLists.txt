cmake_minimum_required(VERSION 3.20)
project(t81_foundation VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(ggml REQUIRED QUIET)

# Core T81 libraries
add_library(t81_core STATIC
  src/codec/base81_packed.cpp
  src/codec/metadata.cpp
  src/codec/trit_packing.cpp
  src/codec/ternary_quantization.cpp
  src/codec/ternary_gguf.cpp
  src/crypto/sha3.cpp
  src/setun/bridge.cpp
)

# T81 ISA library
add_library(t81_isa STATIC
  core/isa/binary_emitter.cpp
  core/isa/binary_io.cpp
  core/isa/base81_view.cpp
)

target_link_libraries(t81_core PUBLIC t81_isa)
target_include_directories(t81_isa PUBLIC include)

# Phase 3 Enhanced libraries
add_library(t81_enhanced_codec STATIC
  src/codec/enhanced_gguf_parser.cpp
  src/codec/advanced_ternary_quantization.cpp
)

add_library(t81_memory STATIC
  src/memory/advanced_memory_manager.cpp
)

add_library(t81_inference STATIC
  src/inference/batch_inference_engine.cpp
)

add_library(t81_monitoring STATIC
  src/monitoring/performance_monitor.cpp
)

# Llama adapter library (updated for Phase 3)
add_library(t81_llama_adapter STATIC
  tooling/model/llama_cpp_adapter_working.cpp
  src/codec/gguf_parser.cpp
  src/codec/ternary_gguf.cpp
  src/axion/policy_engine.cpp
)

# Link libraries
target_link_libraries(t81_enhanced_codec PUBLIC t81_core t81_isa)
target_link_libraries(t81_memory PUBLIC t81_core t81_isa)
target_link_libraries(t81_inference PUBLIC t81_core t81_isa t81_enhanced_codec t81_memory)
target_link_libraries(t81_monitoring PUBLIC t81_core t81_isa)
target_link_libraries(t81_llama_adapter PUBLIC t81_core t81_isa t81_enhanced_codec t81_memory ggml::ggml)

# Include directories
target_include_directories(t81_enhanced_codec PUBLIC include)
target_include_directories(t81_memory PUBLIC include)
target_include_directories(t81_inference PUBLIC include)
target_include_directories(t81_monitoring PUBLIC include)
target_include_directories(t81_llama_adapter PUBLIC include tooling/model third_party/llama.cpp)

# Phase 3 Integration Example
add_executable(phase3_integration_demo
  examples/ai-integration/phase3_complete_integration_demo.cpp
)

target_link_libraries(phase3_integration_demo 
  PUBLIC 
  t81_core 
  t81_isa 
  t81_enhanced_codec 
  t81_memory 
  t81_inference 
  t81_monitoring 
  t81_llama_adapter
  ggml::ggml
)

# AI integration examples
add_executable(llama_runner_real examples/ai-integration/llama_real_simple_restored.cpp)
target_link_libraries(llama_runner_real PRIVATE t81_core t81_isa t81_llama_adapter)

add_executable(llama_cpp_governed_demo examples/llama_cpp_governed_demo.cpp)
target_link_libraries(llama_cpp_governed_demo PRIVATE t81_core t81_isa t81_llama_adapter)

# Installation
install(TARGETS t81_core t81_isa t81_llama_adapter llama_runner_real)
install(DIRECTORY include/ DESTINATION include/t81)
