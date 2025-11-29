#include "t81/core/T81Tree.hpp"
#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81Tree tests...\n";

    // Leaf node
    auto leaf = T81Tree<T81Int<27>>::leaf(T81Int<27>(42));
    assert(leaf->is_leaf());
    assert(leaf->value().to_int64() == 42);
    assert(leaf->left() == nullptr);
    assert(leaf->middle() == nullptr);
    assert(leaf->right() == nullptr);

    // Node with children
    auto left_child = T81Tree<T81Int<27>>::leaf(T81Int<27>(10));
    auto right_child = T81Tree<T81Int<27>>::leaf(T81Int<27>(30));
    auto node = T81Tree<T81Int<27>>::node(
        T81Int<27>(20),
        left_child,
        std::nullopt,
        right_child
    );

    assert(!node->is_leaf());
    assert(node->value().to_int64() == 20);
    assert(node->left() != nullptr);
    assert(node->left()->value().to_int64() == 10);
    assert(node->right() != nullptr);
    assert(node->right()->value().to_int64() == 30);
    assert(node->middle() == nullptr);

    // Persistent update
    auto new_left = T81Tree<T81Int<27>>::leaf(T81Int<27>(15));
    auto updated = node->with_left(std::move(new_left));
    assert(updated->left()->value().to_int64() == 15);
    // Original should be unchanged (persistent)
    assert(node->left()->value().to_int64() == 10);

    std::cout << "All T81Tree tests PASSED!\n";
    return 0;
}

