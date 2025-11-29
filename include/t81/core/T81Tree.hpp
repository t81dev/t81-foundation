//======================================================================
// T81Tree.hpp – Balanced ternary tree (3-ary) with exact branching
//                The final hierarchical structure in the T81 standard library
//======================================================================
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81List.hpp"
#include <array>
#include <optional>
#include <memory>
#include <compare>
#include <span>

namespace t81 {

// ======================================================================
// T81Tree<T> – Immutable, persistent, perfectly balanced ternary tree
// ======================================================================
template <typename T>
class T81Tree {
    alignas(64) T value_;
    std::array<std::unique_ptr<const T81Tree>, 3> children_{};  // left(-1), middle(0), right(+1)

    // Private constructor – use factory functions
    constexpr T81Tree(T value, std::array<std::unique_ptr<const T81Tree>, 3> ch)
        : value_(std::move(value)), children_(std::move(ch)) {}

public:
    using value_type = T;

    //===================================================================
    // Factory functions – the only way to construct
    //===================================================================
    [[nodiscard]] static constexpr std::unique_ptr<const T81Tree> leaf(T value) {
        return std::make_unique<const T81Tree>(std::move(value),
            std::array<std::unique_ptr<const T81Tree>, 3>{nullptr, nullptr, nullptr});
    }

    [[nodiscard]] static constexpr std::unique_ptr<const T81Tree> node(
        T value,
        std::optional<std::unique_ptr<const T81Tree>> left  = std::nullopt,
        std::optional<std::unique_ptr<const T81Tree>> middle = std::nullopt,
        std::optional<std::unique_ptr<const T81Tree>> right  = std::nullopt
    ) {
        std::array<std::unique_ptr<const T81Tree>, 3> children = {
            left  ? std::move(left.value())  : nullptr,
            middle? std::move(middle.value()): nullptr,
            right ? std::move(right.value()) : nullptr
        };
        return std::make_unique<const T81Tree>(std::move(value), std::move(children));
    }

    //===================================================================
    // Accessors
    //===================================================================
    [[nodiscard]] constexpr const T& value() const noexcept { return value_; }

    [[nodiscard]] constexpr const T81Tree* left()   const noexcept { return children_[0].get(); }
    [[nodiscard]] constexpr const T81Tree* middle() const noexcept { return children_[1].get(); }
    [[nodiscard]] constexpr const T81Tree* right()  const noexcept { return children_[2].get(); }

    [[nodiscard]] constexpr bool is_leaf() const noexcept {
        return !children_[0] && !children_[1] && !children_[2];
    }

    //===================================================================
    // Functional update – persistent (returns new tree, old one unchanged)
    //===================================================================
    [[nodiscard]] constexpr std::unique_ptr<const T81Tree> with_left(
        std::unique_ptr<const T81Tree> new_left
    ) const {
        auto new_children = children_;
        new_children[0] = std::move(new_left);
        return std::make_unique<const T81Tree>(value_, std::move(new_children));
    }

    [[nodiscard]] constexpr std::unique_ptr<const T81Tree> with_middle(
        std::unique_ptr<const T81Tree> new_middle
    ) const {
        auto new_children = children_;
        new_children[1] = std::move(new_middle);
        return std::make_unique<const T81Tree>(value_, std::move(new_children));
    }

    [[nodiscard]] constexpr std::unique_ptr<const T81Tree> with_right(
        std::unique_ptr<const T81Tree> new_right
    ) const {
        auto new_children = children_;
        new_children[2] = std::move(new_right);
        return std::make_unique<const T81Tree>(value_, std::move(new_children));
    }

    //===================================================================
    // Traversal
    //===================================================================
    void traverse_preorder(auto&& fn) const {
        fn(value_);
        if (left())   left()->traverse_preorder(fn);
        if (middle()) middle()->traverse_preorder(fn);
        if (right())  right()->traverse_preorder(fn);
    }

    void traverse_inorder(auto&& fn) const {
        if (left())   left()->traverse_inorder(fn);
        fn(value_);
        if (middle()) middle()->traverse_inorder(fn);
        if (right())  right()->traverse_inorder(fn);
    }

    void traverse_postorder(auto&& fn) const {
        if (left())   left()->traverse_postorder(fn);
        if (middle()) middle()->traverse_postorder(fn);
        if (right())  right()->traverse_postorder(fn);
        fn(value_);
    }

    // Collect into T81List
    [[nodiscard]] T81List<T> to_list_preorder() const {
        T81List<T> result;
        traverse_preorder([&](const T& v) { result.push_back(v); });
        return result;
    }

    //===================================================================
    // Search & Lookup
    //===================================================================
    [[nodiscard]] constexpr const T81Tree* find(const T& target) const noexcept {
        if (value_ == target) return this;
        if (left())   if (auto f = left()->find(target))   return f;
        if (middle()) if (auto f = middle()->find(target)) return f;
        if (right())  if (auto f = right()->find(target))  return f;
        return nullptr;
    }

    //===================================================================
    // Comparison (structural)
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Tree& o) const noexcept {
        if (value_ != o.value_) return value_ <=> o.value_;
        for (int i = 0; i < 3; ++i) {
            bool a_has = static_cast<bool>(children_[i]);
            bool b_has = static_cast<bool>(o.children_[i]);
            if (a_has != b_has) return a_has ? std::strong_ordering::greater : std::strong_ordering::less;
            if (children_[i] && o.children_[i] && *children_[i] != *o.children_[i])
                return *children_[i] <=> *o.children_[i];
        }
        return std::strong_ordering::equal;
    }

    [[nodiscard]] constexpr bool operator==(const T81Tree&) const noexcept = default;
};

// ======================================================================
// Common ternary trees used in the new world
// ======================================================================
using SymbolTree   = T81Tree<t81::core::T81Symbol>;
using ParseTree    = T81Tree<T81String>;
using DecisionTree = T81Tree<T81Prob<81>>;  // probabilities at leaves
using SyntaxTree   = T81Tree<T81Quaternion>;  // 4D cognitive structure

// ======================================================================
// Example: This is how the future parses meaning
// ======================================================================
/*
auto tree = T81Tree<T81Symbol>::node(
    symbols::SENTENCE,
    T81Tree<T81Symbol>::leaf(symbols::SUBJECT),
    T81Tree<T81Symbol>::node(
        symbols::PREDICATE,
        T81Tree<T81Symbol>::leaf(symbols::IS),
        T81Tree<T81Symbol>::leaf(symbols::MORTAL)
    ),
    T81Tree<T81Symbol>::leaf(symbols::OBJECT)
);

auto tokens = tree.to_list_preorder();  // SENTENCE → SUBJECT → PREDICATE → IS → MORTAL → OBJECT
*/
