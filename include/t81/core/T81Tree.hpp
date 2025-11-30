/**
 * @file T81Tree.hpp
 * @brief Immutable ternary tree (left, middle, right) with shared structure.
 *
 * Persistent API:
 *   • using ptr  = std::shared_ptr<const T81Tree<T>>;
 *   • static ptr leaf(value);
 *   • static ptr node(value, children_array);
 *   • static ptr node(value, left, middle, right);
 *   • static ptr node(value, opt_left, opt_middle, opt_right);
 *   • with_left / with_middle / with_right → updated ptrs sharing structure.
 */

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace t81 {

template <typename T>
class T81Tree {
public:
    using value_type     = T;
    using node_type      = T81Tree<T>;
    using ptr            = std::shared_ptr<const node_type>;
    using children_array = std::array<ptr, 3>; // 0 = left, 1 = middle, 2 = right

private:
    value_type     value_{};
    children_array children_{};  // default-initialised to {nullptr, nullptr, nullptr}

public:
    //===================================================================
    // Constructors – public so std::make_shared can construct nodes
    //===================================================================

    T81Tree() = default;

    T81Tree(const value_type& v, children_array children = {})
        : value_(v)
        , children_(std::move(children)) {}

    T81Tree(value_type&& v, children_array children = {})
        : value_(std::move(v))
        , children_(std::move(children)) {}

    //===================================================================
    // Construction helpers
    //===================================================================

    // Leaf with no children
    static ptr leaf(const value_type& v) {
        return std::make_shared<node_type>(v, children_array{});
    }

    static ptr leaf(value_type&& v) {
        return std::make_shared<node_type>(std::move(v), children_array{});
    }

    // Node with explicit children array
    static ptr node(const value_type& v, children_array children) {
        return std::make_shared<node_type>(v, std::move(children));
    }

    static ptr node(value_type&& v, children_array children) {
        return std::make_shared<node_type>(std::move(v), std::move(children));
    }

    // Node with raw child pointers
    static ptr node(const value_type& v, ptr left, ptr middle, ptr right) {
        children_array children{ std::move(left), std::move(middle), std::move(right) };
        return std::make_shared<node_type>(v, std::move(children));
    }

    static ptr node(value_type&& v, ptr left, ptr middle, ptr right) {
        children_array children{ std::move(left), std::move(middle), std::move(right) };
        return std::make_shared<node_type>(std::move(v), std::move(children));
    }

    // Node with optional child pointers (matches test_T81Tree.cpp)
    static ptr node(
        const value_type& v,
        std::optional<ptr> left,
        std::optional<ptr> middle,
        std::optional<ptr> right
    ) {
        children_array children{
            left   ? *left   : ptr{},
            middle ? *middle : ptr{},
            right  ? *right  : ptr{}
        };
        return std::make_shared<node_type>(v, std::move(children));
    }

    static ptr node(
        value_type&& v,
        std::optional<ptr> left,
        std::optional<ptr> middle,
        std::optional<ptr> right
    ) {
        children_array children{
            left   ? *left   : ptr{},
            middle ? *middle : ptr{},
            right  ? *right  : ptr{}
        };
        return std::make_shared<node_type>(std::move(v), std::move(children));
    }

    //===================================================================
    // Observers
    //===================================================================

    [[nodiscard]] const value_type& value() const noexcept {
        return value_;
    }

    [[nodiscard]] const children_array& children() const noexcept {
        return children_;
    }

    [[nodiscard]] ptr left() const noexcept   { return children_[0]; }
    [[nodiscard]] ptr middle() const noexcept { return children_[1]; }
    [[nodiscard]] ptr right() const noexcept  { return children_[2]; }

    //===================================================================
    // Persistent update helpers
    //===================================================================

    [[nodiscard]] ptr with_left(ptr new_left) const {
        children_array c = children_;
        c[0] = std::move(new_left);
        return std::make_shared<node_type>(value_, std::move(c));
    }

    [[nodiscard]] ptr with_middle(ptr new_middle) const {
        children_array c = children_;
        c[1] = std::move(new_middle);
        return std::make_shared<node_type>(value_, std::move(c));
    }

    [[nodiscard]] ptr with_right(ptr new_right) const {
        children_array c = children_;
        c[2] = std::move(new_right);
        return std::make_shared<node_type>(value_, std::move(c));
    }
};

} // namespace t81
