#include "t81/types/T81Map.hpp"
#include "t81/types/T81Symbol.hpp"
#include "t81/types/T81String.hpp"
#include <cassert>
#include <iostream>
#include <vector>

// Test T81Map determinism guarantees
// This test ensures that map iteration and serialization are deterministic
// even when internal hash-based structures may have non-deterministic ordering

void test_map_determinism() {
    // Test with T81Symbol keys (should use perfect hash)
    {
        t81::T81Map<t81::T81Symbol, int> symbol_map;
        
        // Insert symbols in different orders
        t81::T81Symbol s1 = t81::T81Symbol::intern("beta");
        t81::T81Symbol s2 = t81::T81Symbol::intern("alpha"); 
        t81::T81Symbol s3 = t81::T81Symbol::intern("gamma");
        
        symbol_map[s3] = 3;
        symbol_map[s1] = 1;
        symbol_map[s2] = 2;
        
        // Test deterministic iteration using deterministic_iterator
        auto items = std::vector<std::pair<t81::T81Symbol, int>>(
            symbol_map.dbegin(), symbol_map.dend()
        );
        assert(items.size() == 3);
        
        // Should be sorted by symbol name: alpha, beta, gamma
        assert(items[0].first == s2 && items[0].second == 2);
        assert(items[1].first == s1 && items[1].second == 1);
        assert(items[2].first == s3 && items[2].second == 3);
        
        // Test deterministic serialization
        std::string serialized = symbol_map.serialize_canonical();
        // Should contain keys in alphabetical order
        assert(serialized.find("alpha") < serialized.find("beta"));
        assert(serialized.find("beta") < serialized.find("gamma"));
        
        std::cout << "✓ T81Symbol map determinism works\n";
    }
    
    // Test with string keys (should use CanonHash)
    {
        t81::T81Map<t81::T81String, int> string_map;
        
        // Insert strings in different orders
        t81::T81String s1 = t81::T81String("zebra");
        t81::T81String s2 = t81::T81String("apple");
        t81::T81String s3 = t81::T81String("banana");
        
        string_map[s3] = 3;
        string_map[s1] = 1;
        string_map[s2] = 2;
        
        // Test deterministic iteration using deterministic_iterator
        auto items = std::vector<std::pair<t81::T81String, int>>(
            string_map.dbegin(), string_map.dend()
        );
        assert(items.size() == 3);
        
        // Should be sorted by string value: apple, banana, zebra
        assert(items[0].first == s2 && items[0].second == 2);
        assert(items[1].first == s3 && items[1].second == 3);
        assert(items[2].first == s1 && items[2].second == 1);
        
        // Test deterministic serialization
        std::string serialized = string_map.serialize_canonical();
        // Should contain keys in alphabetical order
        assert(serialized.find("apple") < serialized.find("banana"));
        assert(serialized.find("banana") < serialized.find("zebra"));
        
        std::cout << "✓ T81String map determinism works\n";
    }
    
    // Test with integer keys
    {
        t81::T81Map<int, t81::T81String> int_map;
        
        // Insert integers in random order
        int_map[42] = t81::T81String(t81::T81String("answer"));
        int_map[7] = t81::T81String(t81::T81String("lucky"));
        int_map[3] = t81::T81String(t81::T81String("pi"));
        int_map[100] = t81::T81String(t81::T81String("century"));
        
        // Test deterministic iteration using deterministic_iterator
        auto items = std::vector<std::pair<int, t81::T81String>>(
            int_map.dbegin(), int_map.dend()
        );
        assert(items.size() == 4);
        
        // Should be sorted by integer key: 3, 7, 42, 100
        assert(items[0].first == 3);
        assert(items[1].first == 7);
        assert(items[2].first == 42);
        assert(items[3].first == 100);
        
        // Test deterministic serialization
        std::string serialized = int_map.serialize_canonical();
        // Should contain keys in numerical order
        assert(serialized.find("3") < serialized.find("7"));
        assert(serialized.find("7") < serialized.find("42"));
        assert(serialized.find("42") < serialized.find("100"));
        
        std::cout << "✓ Integer map determinism works\n";
    }
    
    // Test deterministic vs non-deterministic iterator consistency
    {
        t81::T81Map<t81::T81String, int> map;
        map[t81::T81String(t81::T81String("x"))] = 1;
        map[t81::T81String(t81::T81String("y"))] = 2;
        map[t81::T81String(t81::T81String("z"))] = 3;
        
        // Multiple calls to deterministic iterator should return the same order
        auto items1 = std::vector<std::pair<t81::T81String, int>>(
            map.dbegin(), map.dend()
        );
        auto items2 = std::vector<std::pair<t81::T81String, int>>(
            map.dbegin(), map.dend()
        );
        auto items3 = map.iter_sorted();
        
        assert(items1.size() == items2.size());
        assert(items2.size() == items3.size());
        
        for (size_t i = 0; i < items1.size(); ++i) {
            assert(items1[i].first == items2[i].first);
            assert(items2[i].first == items3[i].first);
            assert(items1[i].second == items2[i].second);
            assert(items2[i].second == items3[i].second);
        }
        
        std::cout << "✓ Deterministic iteration consistency works\n";
    }
    
    // Test empty map determinism
    {
        t81::T81Map<t81::T81String, int> empty_map;
        
        auto items = std::vector<std::pair<t81::T81String, int>>(
            empty_map.dbegin(), empty_map.dend()
        );
        assert(items.empty());
        
        std::string serialized = empty_map.serialize_canonical();
        assert(serialized == "{}");
        
        std::cout << "✓ Empty map determinism works\n";
    }
    
    // Test single element map determinism
    {
        t81::T81Map<t81::T81String, int> single_map;
        single_map[t81::T81String(t81::T81String("only"))] = 42;
        
        auto items = std::vector<std::pair<t81::T81String, int>>(
            single_map.dbegin(), single_map.dend()
        );
        assert(items.size() == 1);
        assert(items[0].first == t81::T81String(t81::T81String("only")));
        assert(items[0].second == 42);
        
        std::string serialized = single_map.serialize_canonical();
        assert(serialized.find("only") != std::string::npos);
        assert(serialized.find("42") != std::string::npos);
        
        std::cout << "✓ Single element map determinism works\n";
    }
    
    std::cout << "\nAll map determinism tests passed!\n";
}

void test_map_hash_robustness() {
    // Test that hash function variations don't affect deterministic output
    t81::T81Map<t81::T81String, int> map1, map2;
    
    // Insert same data in different orders
    t81::T81String keys[] = {t81::T81String("omega"), t81::T81String("alpha"), t81::T81String("beta"), t81::T81String("gamma"), t81::T81String("delta")};
    int values[] = {5, 1, 2, 3, 4};
    
    // Insert in order
    for (int i = 0; i < 5; ++i) {
        map1[keys[i]] = values[i];
    }
    
    // Insert in reverse order
    for (int i = 4; i >= 0; --i) {
        map2[keys[i]] = values[i];
    }
    
    // Both should serialize to the same canonical form
    std::string serialized1 = map1.serialize_canonical();
    std::string serialized2 = map2.serialize_canonical();
    
    assert(serialized1 == serialized2);
    
    // Both should iterate to the same sorted order
    auto items1 = map1.iter_sorted();
    auto items2 = map2.iter_sorted();
    
    assert(items1.size() == items2.size());
    for (size_t i = 0; i < items1.size(); ++i) {
        assert(items1[i].first == items2[i].first);
        assert(items1[i].second == items2[i].second);
    }
    
    std::cout << "✓ Hash robustness test passed!\n";
}

int main() {
    test_map_determinism();
    test_map_hash_robustness();
    return 0;
}
