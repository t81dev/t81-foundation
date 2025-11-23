#include <cassert>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "t81/t81.hpp"
#include "t81/fraction.hpp"

int main(){
  using json = nlohmann::json;
  std::ifstream f("tests/harness/canonical/fraction.json");
  if(!f){ std::cerr << "missing fraction.json\n"; return 1; }
  json J; f >> J;

  for (auto& tc : J["cases"]) {
    auto an = tc["a_num"].get<std::string>();
    auto ad = tc["a_den"].get<std::string>();
    auto bn = tc["b_num"].get<std::string>();
    auto bd = tc["b_den"].get<std::string>();

    t81::T81Fraction A = t81::T81Fraction::make(an, ad);
    t81::T81Fraction B = t81::T81Fraction::make(bn, bd);

    auto S = t81::T81Fraction::add(A, B).to_string();
    auto P = t81::T81Fraction::mul(A, B).to_string();

    // Prefer reduced expectations if present; fall back to generic fields; else sanity-check non-empty.
    if (tc.contains("sum_reduced"))      { assert(S == tc["sum_reduced"].get<std::string>()); }
    else if (tc.contains("sum"))         { assert(S == tc["sum"].get<std::string>()); }
    else                                 { assert(!S.empty()); }

    if (tc.contains("prod_reduced"))     { assert(P == tc["prod_reduced"].get<std::string>()); }
    else if (tc.contains("prod"))        { assert(P == tc["prod"].get<std::string>()); }
    else                                 { assert(!P.empty()); }
  }

  std::cout << "fraction ok\n";
}
