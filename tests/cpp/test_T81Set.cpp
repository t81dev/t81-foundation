#include "t81/core/T81Set.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cassert>
#include <iostream>

using namespace t81;

int main() {
    std::cout << "Running T81Set tests...\n";

    // Construction
    T81Set<T81Symbol> empty;
    assert(empty.empty());
    assert(empty.size() == 0);

    T81Symbol sym1 = T81Symbol::intern("sym1");
    T81Symbol sym2 = T81Symbol::intern("sym2");
    T81Symbol sym3 = T81Symbol::intern("sym3");

    // Insert (functional style - returns new set)
    T81Set<T81Symbol> set1 = empty.insert(sym1);
    assert(set1.size() == 1);
    assert(set1.contains(sym1));

    T81Set<T81Symbol> set2 = set1.insert(sym2);
    assert(set2.size() == 2);
    assert(set2.contains(sym1));
    assert(set2.contains(sym2));

    // Initializer list
    T81Set<T81Symbol> set3{sym1, sym2, sym3};
    assert(set3.size() == 3);

    // Erase
    T81Set<T81Symbol> set4 = set3.erase(sym2);
    assert(set4.size() == 2);
    assert(!set4.contains(sym2));
    assert(set4.contains(sym1));
    assert(set4.contains(sym3));

    // Union
    T81Set<T81Symbol> set5{sym1};
    T81Set<T81Symbol> set6{sym2};
    T81Set<T81Symbol> union_set = set5 | set6;
    assert(union_set.size() == 2);
    assert(union_set.contains(sym1));
    assert(union_set.contains(sym2));

    // Intersection
    T81Set<T81Symbol> set7{sym1, sym2};
    T81Set<T81Symbol> set8{sym2, sym3};
    T81Set<T81Symbol> inter = set7 & set8;
    assert(inter.size() == 1);
    assert(inter.contains(sym2));

    std::cout << "All T81Set tests PASSED!\n";
    return 0;
}

