#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "t81/fraction.hpp"
#include "t81/tensor.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/tisc/type_alias.hpp"
#include "t81/weights.hpp"

namespace t81::tisc {
enum class LiteralKind : std::uint8_t {
  Int = 0,
  FloatHandle,
  FractionHandle,
  SymbolHandle,
  TensorHandle,
  ShapeHandle,
};

struct Insn {
  Opcode opcode{Opcode::Nop};
  std::int32_t a{0};
  std::int32_t b{0};
  std::int32_t c{0};
  LiteralKind literal_kind{LiteralKind::Int};
};

struct Program {
  std::vector<Insn> insns;
  std::vector<double> float_pool;
  std::vector<t81::T81Fraction> fraction_pool;
  std::vector<std::string> symbol_pool;
  std::vector<t81::T729Tensor> tensor_pool;
  std::vector<std::vector<int>> shape_pool;
  std::string axion_policy_text;
  std::string match_metadata_text;
  std::shared_ptr<t81::weights::ModelFile> weights_model;
  std::vector<tisc::TypeAliasMetadata> type_aliases;
};
}  // namespace t81::tisc
