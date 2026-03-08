#include "t81/frontend/builtin_registry.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

namespace t81::frontend {

namespace {

// Thread-safe lazy-init lookup maps over kBuiltinTable.
struct Registry {
  std::unordered_map<std::string_view, const BuiltinDef*> by_stdlib;
  std::unordered_map<std::string_view, const BuiltinDef*> by_canonical;

  Registry() {
    for (const BuiltinDef& def : kBuiltinTable) {
      by_stdlib.emplace(def.stdlib_name, &def);
      // First entry wins for canonical lookup (many-to-one aliases).
      by_canonical.emplace(def.canonical, &def);
    }
  }
};

const Registry& registry() {
  static Registry r;
  return r;
}

}  // namespace

std::string_view canonical_name_for(std::string_view stdlib_name) noexcept {
  const auto& reg = registry();
  auto it = reg.by_stdlib.find(stdlib_name);
  if (it != reg.by_stdlib.end()) {
    return it->second->canonical;
  }
  return stdlib_name;  // unknown name: pass through unchanged (preserves existing behaviour)
}

const BuiltinDef* lookup_builtin(std::string_view stdlib_name) noexcept {
  const auto& reg = registry();
  auto it = reg.by_stdlib.find(stdlib_name);
  return it != reg.by_stdlib.end() ? it->second : nullptr;
}

const BuiltinDef* lookup_builtin_by_canonical(std::string_view canonical) noexcept {
  const auto& reg = registry();
  auto it = reg.by_canonical.find(canonical);
  return it != reg.by_canonical.end() ? it->second : nullptr;
}

}  // namespace t81::frontend
