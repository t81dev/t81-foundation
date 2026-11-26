add_library(t81_crypto STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/ed25519_bundle.cpp
)
t81_preserve_root_output(t81_crypto)
target_include_directories(t81_crypto PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
target_compile_features(t81_crypto PUBLIC ${T81_CXX_STD_FEATURE})
