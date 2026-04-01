add_library(t81_axion STATIC
  ${PROJECT_SOURCE_DIR}/kernel/axion/axion_api.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/engine.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/ethics.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/nondeterminism_detector.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/policy_engine.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/policy_serialization.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/canonfs_hook.cpp
  # RFC-0032 Phase 3 -- promoted AI governance components
  ${PROJECT_SOURCE_DIR}/kernel/axion/ai_hooks.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/ai_model_loader.cpp
  ${PROJECT_SOURCE_DIR}/kernel/axion/policy_validator.cpp
)
t81_preserve_root_output(t81_axion)
target_link_libraries(t81_axion PUBLIC t81_core)
