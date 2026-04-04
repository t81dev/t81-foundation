# Controlled Stochastic Inference (CSI) Subsystem
# EXPERIMENTAL - NOT FOR PRODUCTION USE

cmake_minimum_required(VERSION 3.16)

# CSI Library
add_library(t81_csi STATIC
    stochastic_decoder.cpp
    stochastic_provenance.cpp
    policy_gated_sampling.cpp
)

target_include_directories(t81_csi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(t81_csi
    PUBLIC
        t81_core
        t81_axion
        t81_canonfs
        t81_vm
    PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
)

# CSI Tools
add_executable(t81_csi_test
    test_csi.cpp
)

target_link_libraries(t81_csi_test
    PRIVATE
        t81_csi
        t81_test_framework
)

# CSI CLI Extensions
add_executable(t81_csi_cli
    csi_cli.cpp
)

target_link_libraries(t81_csi_cli
    PRIVATE
        t81_csi
        t81_cli_core
)

# Installation
install(TARGETS t81_csi
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    stochastic_decoder.hpp
    stochastic_provenance.hpp
    policy_gated_sampling.hpp
    DESTINATION include/t81/experimental/csi
)

# Testing
enable_testing()
add_test(NAME csi_unit_tests COMMAND t81_csi_test)
add_test(NAME csi_integration_tests COMMAND t81_csi_test --integration)

# Experimental Build Flag
option(T81_BUILD_EXPERIMENTAL_CSI "Build Controlled Stochastic Inference subsystem" OFF)

if(NOT T81_BUILD_EXPERIMENTAL_CSI)
    target_compile_definitions(t81_csi PRIVATE T81_EXPERIMENTAL_ONLY)
    message(WARNING "CSI subsystem is experimental and not for production use")
endif()

# Dependencies
find_package(Threads REQUIRED)
