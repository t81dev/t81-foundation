#include <new>
#include <cstdlib>
#include <cstring>
#include <exception>

#include "t81/bigint.hpp"
#include "t81/t81.hpp"
#include "t81/config.hpp"
#include "c_api/t81_c_api.h"

using t81::T243BigInt;

struct t81_bigint_s {
  T243BigInt* p;
};

extern "C" {

t81_bigint t81_bigint_from_ascii(const char* s) {
  try {
    if (!s) return nullptr;
    t81_bigint h = reinterpret_cast<t81_bigint>(std::malloc(sizeof(*h)));
    if (!h) return nullptr;
    h->p = new (std::nothrow) T243BigInt(T243BigInt::from_ascii(std::string(s)));
    if (!h->p) { std::free(h); return nullptr; }
    return h;
  } catch (...) {
    return nullptr;
  }
}

char* t81_bigint_to_string(t81_bigint h) {
  try {
    if (!h || !h->p) return nullptr;
    std::string s = h->p->to_string();
    char* out = static_cast<char*>(std::malloc(s.size()+1));
    if (!out) return nullptr;
    std::memcpy(out, s.c_str(), s.size()+1);
    return out;
  } catch (...) {
    return nullptr;
  }
}

void t81_bigint_free(t81_bigint h) {
  if (!h) return;
  try {
    delete h->p;
    h->p = nullptr;
  } catch (...) {
    // swallow
  }
  std::free(h);
}

} // extern "C"
