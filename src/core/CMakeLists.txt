add_library(t81_core STATIC
  ${PROJECT_SOURCE_DIR}/isa/types/bigint.cpp
  ${PROJECT_SOURCE_DIR}/isa/types/fraction.cpp
  ${PROJECT_SOURCE_DIR}/isa/encoding.cpp
  ${PROJECT_SOURCE_DIR}/fs/canonfs_interchange.cpp
  ${PROJECT_SOURCE_DIR}/fs/canonfs_interchange_ops.cpp
  ${PROJECT_SOURCE_DIR}/fs/in_memory_driver.cpp
  ${PROJECT_SOURCE_DIR}/fs/persistent_driver.cpp
  ${PROJECT_SOURCE_DIR}/fs/block_backed_driver.cpp
  ${PROJECT_SOURCE_DIR}/vm/determinism/canon_hash81.cpp
  ${PROJECT_SOURCE_DIR}/isa/math/t81_soft_math/t81_soft_math.cpp
  ${PROJECT_SOURCE_DIR}/codec/base243.cpp
  ${PROJECT_SOURCE_DIR}/codec/base81.cpp
  ${PROJECT_SOURCE_DIR}/codec/base81_balanced.cpp
  ${PROJECT_SOURCE_DIR}/codec/base81_packed.cpp
  ${PROJECT_SOURCE_DIR}/codec/metadata.cpp
  ${PROJECT_SOURCE_DIR}/codec/trit_packing.cpp
  ${PROJECT_SOURCE_DIR}/experimental/cog/v1/symbolic_graph.cpp
  ${PROJECT_SOURCE_DIR}/crypto/sha3.cpp
  ${PROJECT_SOURCE_DIR}/experimental/setun/bridge.cpp
)
t81_preserve_root_output(t81_core)
add_library(t81_types ALIAS t81_core)
target_compile_features(t81_core PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_core PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# block_backed_driver.cpp needs access to dev/block_device.hpp and dev/canon_store.hpp.
target_include_directories(t81_core PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ternaryos>
)

add_library(t81_tracing OBJECT
  ${PROJECT_SOURCE_DIR}/vm/tracing/canonhash81.cpp
)
target_compile_features(t81_tracing PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_tracing PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
target_sources(t81_core PRIVATE $<TARGET_OBJECTS:t81_tracing>)

if(T81_IS_X86_64)
  set(T81_AVX2_SOURCE ${PROJECT_SOURCE_DIR}/inference/simd/base81_digits_avx2.cpp)
  target_sources(t81_core PRIVATE ${T81_AVX2_SOURCE})
  if(NOT MSVC)
    set_source_files_properties(${T81_AVX2_SOURCE} PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")
  endif()
endif()
