#include "t81/frontend/symbol_table.hpp"

namespace t81 {
namespace frontend {

SymbolTable::SymbolTable() {
    // Start with a single global scope.
    enter_scope();
}

void SymbolTable::enter_scope() {
    _scopes.emplace_back();
}

void SymbolTable::exit_scope() {
    if (!_scopes.empty()) {
        _scopes.pop_back();
    }
}

void SymbolTable::define(std::string_view name, Symbol symbol) {
    if (!_scopes.empty()) {
        _scopes.back()[std::string(name)] = symbol;
    }
}

std::optional<Symbol> SymbolTable::lookup(std::string_view name) const {
    for (auto it = _scopes.rbegin(); it != _scopes.rend(); ++it) {
        auto found = it->find(std::string(name));
        if (found != it->end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

} // namespace frontend
} // namespace t81
