#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace t81 {
namespace tisc {

enum class StructuralKind : std::uint8_t {
    TypeAlias = 0,
    Record,
    Enum
};

struct FieldInfo {
    std::string name;
    std::string type;
};

struct VariantInfo {
    std::string name;
    std::optional<std::string> payload;
};

struct TypeAliasMetadata {
    std::string name;
    std::vector<std::string> params;
    std::string alias;
    StructuralKind kind = StructuralKind::TypeAlias;
    std::vector<FieldInfo> fields;
    std::vector<VariantInfo> variants;
};

} // namespace tisc
} // namespace t81
