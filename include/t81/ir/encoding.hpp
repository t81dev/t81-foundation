#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring>
#include "t81/ir/insn.hpp"

namespace t81::ir {

// Fixed 32-byte little-endian layout:
//
// 0x00: uint16  op
// 0x02: uint16  pad (0)
// 0x04: uint32  ops[0]
// 0x08: uint32  ops[1]
// 0x0C: uint32  ops[2]
// 0x10: uint64  imm
// 0x18: uint32  flags
// 0x1C: uint32  _reserved (0)
//
// All fields are encoded little-endian regardless of host endianness.

inline void encode(const Insn& i, uint8_t out[32]) {
  std::memset(out, 0, 32);
  auto wr16 = [&](size_t off, uint16_t v){
    out[off+0] = static_cast<uint8_t>( v        & 0xFF);
    out[off+1] = static_cast<uint8_t>((v >> 8)  & 0xFF);
  };
  auto wr32 = [&](size_t off, uint32_t v){
    out[off+0] = static_cast<uint8_t>( v        & 0xFF);
    out[off+1] = static_cast<uint8_t>((v >> 8)  & 0xFF);
    out[off+2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    out[off+3] = static_cast<uint8_t>((v >> 24) & 0xFF);
  };
  auto wr64 = [&](size_t off, uint64_t v){
    for (int k = 0; k < 8; ++k) out[off+k] = static_cast<uint8_t>((v >> (8*k)) & 0xFF);
  };

  wr16(0x00, static_cast<uint16_t>(i.op));
  wr32(0x04, i.ops[0]);
  wr32(0x08, i.ops[1]);
  wr32(0x0C, i.ops[2]);
  wr64(0x10, i.imm);
  wr32(0x18, i.flags);
  // 0x1C reserved already zeroed
}

inline Insn decode(const uint8_t in[32]) {
  auto rd16 = [&](size_t off)->uint16_t{
    return static_cast<uint16_t>(in[off+0]) |
           static_cast<uint16_t>(in[off+1]) << 8;
  };
  auto rd32 = [&](size_t off)->uint32_t{
    return  (uint32_t)in[off+0]
          | ((uint32_t)in[off+1] << 8)
          | ((uint32_t)in[off+2] << 16)
          | ((uint32_t)in[off+3] << 24);
  };
  auto rd64 = [&](size_t off)->uint64_t{
    uint64_t v = 0;
    for (int k = 0; k < 8; ++k) v |= (uint64_t)in[off+k] << (8*k);
    return v;
  };

  Insn i;
  i.op      = static_cast<Opcode>(rd16(0x00));
  i.ops[0]  = rd32(0x04);
  i.ops[1]  = rd32(0x08);
  i.ops[2]  = rd32(0x0C);
  i.imm     = rd64(0x10);
  i.flags   = rd32(0x18);
  i._reserved = 0;
  return i;
}

inline std::vector<uint8_t> encode_many(const std::vector<Insn>& prog) {
  std::vector<uint8_t> out;
  out.resize(prog.size() * 32);
  for (size_t i = 0; i < prog.size(); ++i) {
    encode(prog[i], &out[i * 32]);
  }
  return out;
}

inline std::vector<Insn> decode_many(const uint8_t* bytes, size_t nbytes) {
  if (nbytes % 32 != 0) throw std::invalid_argument("decode_many: length not multiple of 32");
  size_t n = nbytes / 32;
  std::vector<Insn> out;
  out.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    out.push_back(decode(&bytes[i * 32]));
  }
  return out;
}

} // namespace t81::ir
