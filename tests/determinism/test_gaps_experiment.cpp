#include <gtest/gtest.h>
#include "t81/types/T81Map.hpp"
#include "t81/types/T81Set.hpp"
#include "t81/types/T81Complex.hpp"
#include "t81/types/T81Float.hpp"

// Metamorphic check for collections
TEST(DeterminismGaps, MapCanonicalization) {
  t81::T81Map<std::string, std::string> map1;
  map1["A"] = "1";
  map1["B"] = "2";
  map1["C"] = "3";

  t81::T81Map<std::string, std::string> map2;
  map2["C"] = "3";
  map2["A"] = "1";
  map2["B"] = "2";

  EXPECT_EQ(map1.serialize_canonical(), map2.serialize_canonical());
  EXPECT_EQ(map1.serialize_canonical(), "{A: 1, B: 2, C: 3}");
}

TEST(DeterminismGaps, SetCanonicalization) {
  t81::T81Set<std::string> set1;
  set1 = set1.insert("A");
  set1 = set1.insert("B");
  set1 = set1.insert("C");

  t81::T81Set<std::string> set2;
  set2 = set2.insert("C");
  set2 = set2.insert("B");
  set2 = set2.insert("A");

  EXPECT_EQ(set1.serialize_canonical(), set2.serialize_canonical());
  EXPECT_EQ(set1.serialize_canonical(), "Set{A, B, C}");
}

// Host dependency check for Complex
TEST(DeterminismGaps, ComplexHostGated) {
#ifdef T81_DETERMINISTIC
  t81::T81Complex<18> c(t81::T81Float<18, 9>::from_double(1.0));
  EXPECT_THROW(t81::exp(c), std::domain_error);
  EXPECT_THROW(t81::log(c), std::domain_error);
  EXPECT_THROW(t81::sqrt(c), std::domain_error);
#endif
}

// Test BigInt bounds restriction
#include "t81/frontend/ir_generator.hpp"
TEST(DeterminismGaps, BigIntGatedPrecision) {
  EXPECT_THROW(t81::frontend::parse_base81_integer_literal("9999999999999999999999999999t81"), std::runtime_error);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
