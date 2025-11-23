#include <cassert>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "t81/t81.hpp"

int main(){
  using json = nlohmann::json;
  std::ifstream f("tests/harness/canonical/bigint.json");
  if(!f){ std::cerr << "missing bigint.json\n"; return 1; }
  json J; f >> J;
  for (auto& tc : J["cases"]) {
    auto a = t81::T243BigInt::from_ascii(tc["a"].get<std::string>());
    auto b = t81::T243BigInt::from_ascii(tc["b"].get<std::string>());
    auto s = t81::T243BigInt::add(a,b).to_string();
    if (tc.contains("sum")) assert(s == tc["sum"].get<std::string>());
  }
  std::cout << "ok\n";
}
