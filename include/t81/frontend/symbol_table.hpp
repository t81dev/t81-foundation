#ifndef T81_FRONTEND_SYMBOL_TABLE_HPP
#define T81_FRONTEND_SYMBOL_TABLE_HPP

#include "t81/tisc/ir.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>

namespace t81 {
namespace frontend {

struct Symbol {
    enum class Type { Variable, Function };
    Type type;
    std::variant<tisc::ir::Register, tisc::ir::Label> location;
};

class SymbolTable {
public:
    SymbolTable();

    void enter_scope();
    void exit_scope();

    void define(std::string_view name, Symbol symbol);
    std::optional<Symbol> lookup(std::string_view name) const;

private:
    using Scope = std::unordered_map<std::string, Symbol>;
    std::vector<Scope> _scopes;
};

} // namespace frontend
} // namespace t81

#endif // T81_FRONTEND_SYMBOL_TABLE_HPP
