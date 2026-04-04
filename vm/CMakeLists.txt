# VM CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Check if this is being included from the main CMakeLists.txt
if(NOT DEFINED T81_MAIN_CMAKE)
    # Standalone build for testing
    project(t81_vm_test)
    
    # Find required packages
    find_package(Threads REQUIRED)
    
    # Create test VM library
    add_library(t81_vm_test STATIC)
    target_sources(t81_vm_test PRIVATE
        vm.cpp
        decode_state.cpp
        decoder.cpp
        memory_segments.cpp
        policy_trace_bridge.cpp
        runtime_state_helpers.cpp
        tensor_helpers.cpp
        gc_helpers.cpp
        tier_limits.cpp
        value_ops.cpp
        ffi_dispatcher.cpp
    )
    
    # Include directories
    target_include_directories(t81_vm_test
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_SOURCE_DIR}/experimental/ai/csi
    )
    
    set(T81_VM_TARGET t81_vm_test)
else()
    # Included from main CMakeLists.txt
    set(T81_VM_TARGET t81_vm)
endif()

# CSI Integration (Experimental)
option(T81_BUILD_CSI_INTEGRATION "Build CSI VM integration (Experimental)" OFF)

if(T81_BUILD_CSI_INTEGRATION)
    # CSI integration sources
    set(CSI_SOURCES
        csi_integration.cpp
        csi_integration.hpp
    )
    
    # Add CSI to VM library
    target_sources(${T81_VM_TARGET} PRIVATE ${CSI_SOURCES})
    
    # Link with CSI subsystem if available
    if(TARGET t81_csi)
        target_link_libraries(${T81_VM_TARGET} PRIVATE t81_csi)
    endif()
    
    # Add compile definitions for experimental status
    target_compile_definitions(${T81_VM_TARGET} PRIVATE T81_CSI_EXPERIMENTAL)
    
    message(STATUS "CSI VM integration enabled (EXPERIMENTAL)")
endif()

# Add CSI headers to include directory
target_include_directories(${T81_VM_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/experimental/ai/csi
)

# Advanced AI VM Integration (RFC-00E2)
option(T81_BUILD_ADVANCED_AI "Enable Advanced AI VM integration (experimental)" OFF)

if(T81_BUILD_ADVANCED_AI)
    message(STATUS "Advanced AI VM integration enabled (EXPERIMENTAL)")
    
    # Add experimental build definition
    target_compile_definitions(${T81_VM_TARGET} PRIVATE T81_BUILD_ADVANCED_AI=1)
    target_compile_definitions(${T81_VM_TARGET} PRIVATE T81_ADVANCED_AI_EXPERIMENTAL=1)
    
    # Advanced AI integration sources
    set(ADVANCED_AI_SOURCES
        advanced_ai_integration.cpp
        advanced_ai_integration.hpp
    )
    
    # Add Advanced AI integration to VM target
    target_sources(${T81_VM_TARGET} PRIVATE ${ADVANCED_AI_SOURCES})
    
    # Advanced AI test sources
    set(ADVANCED_AI_TEST_SOURCES
        test_advanced_ai_integration.cpp
        test_advanced_ai_syntax.cpp
        test_real_neural_network.cpp
        benchmark_advanced_ai.cpp
        test_convolutional_integration.cpp
        test_batch_processing.cpp
        test_bundle_batch_integration.cpp
        test_model_checkpoint_bundles.cpp
    )
    
    # Create Advanced AI test executable
    add_executable(t81_advanced_ai_test ${ADVANCED_AI_TEST_SOURCES})
    
    # Link Advanced AI test with required libraries
    if(TARGET t81_axion)
        target_link_libraries(t81_advanced_ai_test PRIVATE t81_axion)
    endif()
    
    if(TARGET t81_canonfs)
        target_link_libraries(t81_advanced_ai_test PRIVATE t81_canonfs)
    endif()
    
    # Add include directories for Advanced AI tests
    target_include_directories(t81_advanced_ai_test
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_SOURCE_DIR}/experimental/ai/csi
    )
    
    # Advanced AI dependencies
    find_package(Threads REQUIRED)
    target_link_libraries(t81_advanced_ai_test PRIVATE Threads::Threads)
    
    # Advanced AI specific compile flags
    target_compile_features(t81_advanced_ai_test PUBLIC cxx_std_17)
    
    # Advanced AI testing
    enable_testing()
    add_test(NAME AdvancedAISyntaxTest COMMAND t81_advanced_ai_test)
    
    message(STATUS "Advanced AI VM integration configured")
endif()
