add_library(t81_io STATIC
  ${PROJECT_SOURCE_DIR}/vm/io/tensor_loader.cpp
)
t81_preserve_root_output(t81_io)
target_link_libraries(t81_io PUBLIC t81_core)
target_include_directories(t81_io PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
