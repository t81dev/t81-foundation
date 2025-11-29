#include "t81/core/T81Map.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81Int.hpp"
#include <cassert>
#include <iostream>

using namespace t81;
using namespace t81::core;

int main() {
    std::cout << "Running T81Map tests...\n";

    // Map with T81Symbol keys
    T81Map<T81Symbol, T81Int<27>> map;

    T81Symbol key1 = T81Symbol::intern("key1");
    T81Symbol key2 = T81Symbol::intern("key2");

    // Insert
    map[key1] = T81Int<27>(100);
    map[key2] = T81Int<27>(200);

    assert(map.size() == 2);
    assert(map[key1].to_int64() == 100);
    assert(map[key2].to_int64() == 200);

    // Contains
    assert(map.contains(key1));
    assert(map.contains(key2));

    // Update
    map[key1] = T81Int<27>(150);
    assert(map[key1].to_int64() == 150);

    // Erase
    map.erase(key2);
    assert(map.size() == 1);
    assert(!map.contains(key2));

    // Get with optional
    T81Symbol key3 = T81Symbol::intern("key3");
    auto opt_val = map.get(key3);
    assert(!opt_val.has_value());
    
    auto opt_val2 = map.get(key1);
    assert(opt_val2.has_value());
    assert(opt_val2.value().to_int64() == 150);

    std::cout << "All T81Map tests PASSED!\n";
    return 0;
}

