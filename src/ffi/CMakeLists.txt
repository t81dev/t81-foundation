# FFI Library - RFC-00B8 Governed Foreign Function Interface
add_library(t81_ffi STATIC
  ${PROJECT_SOURCE_DIR}/ffi/ffi_dispatcher.cpp
)
t81_preserve_root_output(t81_ffi)
target_include_directories(t81_ffi PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(t81_ffi PUBLIC t81_core)

add_library(t81_c_api STATIC
  ${PROJECT_SOURCE_DIR}/ffi/c_api/t81_c_api.cpp
)
t81_preserve_root_output(t81_c_api)
target_link_libraries(t81_c_api PUBLIC t81_core)
target_include_directories(t81_c_api
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${PROJECT_SOURCE_DIR}
)
