function(t81_preserve_root_output target)
  get_target_property(_t81_target_type ${target} TYPE)
  if(NOT _t81_target_type STREQUAL "INTERFACE_LIBRARY" AND NOT _t81_target_type STREQUAL "OBJECT_LIBRARY")
    set_target_properties(${target} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    )
  endif()
endfunction()

add_subdirectory(core)
add_subdirectory(experimental)
add_subdirectory(isa)
add_subdirectory(axion)
add_subdirectory(ffi)
add_subdirectory(io)
add_subdirectory(lang)
add_subdirectory(vm)
add_subdirectory(tools)
add_subdirectory(direct_api)
