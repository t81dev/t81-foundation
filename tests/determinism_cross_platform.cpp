#include <cassert>
#include <iostream>
#include <string>

#define T81_DETERMINISTIC 1

#include "t81/types/T81Entropy.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Map.hpp"
#include "t81/types/T81Set.hpp"
#include "t81/types/T81Symbol.hpp"
#include "t81/types/T81Time.hpp"

void test_map_insertion_order() {
  t81::T81Map<std::string, int> map1;
  map1["c"] = 3;
  map1["a"] = 1;
  map1["b"] = 2;

  t81::T81Map<std::string, int> map2;
  map2["a"] = 1;
  map2["b"] = 2;
  map2["c"] = 3;

  assert(map1.serialize_canonical() == map2.serialize_canonical());
  std::cout << "test_map_insertion_order passed.\n";
}

void test_set_insertion_order() {
  t81::T81Set<std::string> set1;
  set1 = set1.insert("c").insert("a").insert("b");

  t81::T81Set<std::string> set2;
  set2 = set2.insert("a").insert("b").insert("c");

  assert(set1.serialize_canonical() == set2.serialize_canonical());
  std::cout << "test_set_insertion_order passed.\n";
}

void test_float_zero_normalization() {
  auto pos_zero = t81::v1::T81Float18_9::from_double(0.0);
  auto neg_zero = t81::v1::T81Float18_9::from_double(-0.0);
  assert(pos_zero.to_canonical_string() == "+0E0" || pos_zero.to_canonical_string() == "-0E0");
  assert(pos_zero.to_canonical_string() == neg_zero.to_canonical_string());
  std::cout << "test_float_zero_normalization passed.\n";
}

void test_disabled_transcendental_enforcement() {
  auto f = t81::v1::T81Float18_9::from_double(1.0);
  bool threw = false;
  try {
    auto r = f.exp();
    (void)r;
  } catch (const std::domain_error& e) {
    threw = true;
  }
  assert(threw);
  std::cout << "test_disabled_transcendental_enforcement passed.\n";
}

void test_entropy_determinism() {
  t81::EntropyPool::global().seed(12345);
  auto e1 = t81::acquire_entropy(t81::T81Symbol::intern("TEST"));
  auto e2 = t81::acquire_entropy(t81::T81Symbol::intern("TEST"));

  t81::EntropyPool::global().seed(12345);
  auto e3 = t81::acquire_entropy(t81::T81Symbol::intern("TEST"));
  auto e4 = t81::acquire_entropy(t81::T81Symbol::intern("TEST"));

  assert(e1.value() == e3.value());
  assert(e2.value() == e4.value());
  std::cout << "test_entropy_determinism passed.\n";
}

void test_time_determinism() {
  auto tp = t81::T81Time::clock::time_point(std::chrono::seconds(100));
  t81::T81Time::set_deterministic_time(tp);

  auto t1 = t81::T81Time::now(t81::T81Symbol::intern("TEST"));
  auto t2 = t81::T81Time::now(t81::T81Symbol::intern("TEST"));

  assert(t1.since(t2).count() == 0);
  std::cout << "test_time_determinism passed.\n";
}

int main() {
  test_map_insertion_order();
  test_set_insertion_order();
  test_float_zero_normalization();
  test_disabled_transcendental_enforcement();
  test_entropy_determinism();
  test_time_determinism();
  std::cout << "All cross platform determinism tests passed.\n";
  return 0;
}
