#pragma once

#include <string>
#include <vector>

namespace t81 {
namespace tisc {

struct TypeAliasMetadata {
    std::string name;
    std::vector<std::string> params;
    std::string alias;
};

} // namespace tisc
} // namespace t81
