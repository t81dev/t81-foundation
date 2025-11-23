#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <stdexcept>
#include "t81/ir/insn.hpp"

namespace t81::ir {

// Binary encoding (little-endian, fixed 32 bytes per Insn):
// [0..1]   : uint16  op
// [2..3]   : uint16  reserved (0)
// [4..7]   : uint32  ops[0]
// [8..11]  : uint32  ops[1]
// [12..15] : uint32  ops[2]
// [16..23] : uint64  imm
// [24..27] : uint32  flags
// [28..31] : uint32  _reserved (0)

inline void encode(const Insn& i, uint8_t out[32]) {
  auto w16 = [&](size_t off, uint16_t v){ out[off+0]=uint8_t(v); out[off+1]=uint8_t(v>>8); };
  auto w32 = [&](size_t off, uint32_t v){
    out[off+0]=uint8_t(v); out[off+1]=uint8_t(v>>8);
    out[off+2]=uint8_t(v>>16); out[off+3]=uint8_t(v>>24);
  };
  auto w64 = [&](size_t off, uint64_t v){
    for (int k=0;k<8;++k) out[off+k]=uint8_t(v>>(8*k));
  };
  std::memset(out, 0, 32);
  w16(0, static_cast<uint16_t>(i.op));
  w32(4,  i.ops[0]);
  w32(8,  i.ops[1]);
  w32(12, i.ops[2]);
  w64(16, i.imm);
  w32(24, i.flags);
  w32(28, 0);
}

inline Insn decode(const uint8_t in[32]) {
  auto r16 = [&](size_t off)->uint16_t{ return uint16_t(in[off]) | (uint16_t(in[off+1])<<8); };
  auto r32 = [&](size_t off)->uint32_t{
    return uint32_t(in[off]) | (uint32_t(in[off+1])<<8) |
           (uint32_t(in[off+2])<<16) | (uint32_t(in[off+3])<<24);
  };
  auto r64 = [&](size_t off)->uint64_t{
    uint64_t v=0; for (int k=0;k<8;++k) v |= (uint64_t(in[off+k])<<(8*k)); return v;
  };

  Insn i{};
  i.op     = static_cast<Opcode>(r16(0));
  i.ops[0] = r32(4);
  i.ops[1] = r32(8);
  i.ops[2] = r32(12);
  i.imm    = r64(16);
  i.flags  = r32(24);
  // ignore [2..3] and [28..31]
  return i;
}

// Vector helpers
inline std::vector<uint8_t> encode_many(const std::vector<Insn>& prog) {
  std::vector<uint8_t> buf;
  buf.resize(prog.size() * 32);
  for (size_t i=0;i<prog.size();++i) encode(prog[i], &buf[i*32]);
  return buf;
}

inline std::vector<Insn> decode_many(const uint8_t* data, size_t len) {
  if (len % 32 != 0) throw std::invalid_argument("ir::decode_many: length not multiple of 32");
  size_t n = len / 32;
  std::vector<Insn> out(n);
  for (size_t i=0;i<n;++i) out[i] = decode(&data[i*32]);
  return out;
}

} // namespace t81::ir
