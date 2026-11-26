add_library(t81_provider STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/provider_governor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dpe_mcp_coordinator.cpp
)
t81_preserve_root_output(t81_provider)
target_include_directories(t81_provider PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
target_link_libraries(t81_provider PUBLIC t81_core t81_axion t81_crypto)

add_library(t81_provider_adapters STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/t81_vm_provider.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/controlled_provider.cpp
)
t81_preserve_root_output(t81_provider_adapters)
target_include_directories(t81_provider_adapters
  PUBLIC ${PROJECT_SOURCE_DIR}/src
  PRIVATE ${PROJECT_SOURCE_DIR}
)
target_link_libraries(t81_provider_adapters PUBLIC t81_provider t81_vm t81_isa)
