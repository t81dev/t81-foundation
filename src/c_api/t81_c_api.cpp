#include <cstring>
#include <string>
#include <memory>
#include "t81/t81.hpp"
#include "t81_c_api.h"

struct t81_bigint_s { t81::T243BigInt v; };

t81_bigint t81_bigint_from_ascii(const char* s){
  auto h = new t81_bigint_s{ t81::T243BigInt::from_ascii(std::string(s?s:"") ) };
  return h;
}

void t81_bigint_free(t81_bigint h){ delete h; }

char* t81_bigint_to_string(t81_bigint h){
  auto str = h->v.to_string();
  char* out = (char*)std::malloc(str.size()+1);
  std::memcpy(out, str.c_str(), str.size()+1);
  return out;
}
