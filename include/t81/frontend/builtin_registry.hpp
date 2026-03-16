#pragma once
// builtin_registry.hpp — Single source of truth for all T81Lang stdlib builtins.
//
// Adding a new builtin: add ONE row to kBuiltinTable. The SA and IRGen pick it up
// automatically for arity/tier/effect/return-type checks (when needs_custom_sa_check=false)
// and for table-driven IR emission (when needs_custom_ir_emit=false).
//
// DO NOT add to canonical_stdlib_call_name, minimum_tier_for_call_surface, or
// is_effect_surface_call — those functions now derive from this table.

#include "t81/frontend/types.hpp"
#include "t81/isa/ir.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace t81::frontend {

// ---------------------------------------------------------------------------
// How IR code is emitted for a given builtin.
// ---------------------------------------------------------------------------
enum class BuiltinIRClass : uint8_t {
  Direct,      // Single instruction; arity determines A/B/C operands.
  Constructor, // Zero-arg: opcode with dest only.
  LoadiSym,    // LOADI + SymbolHandle (ir_sym_literal is the string key).
  LoadiImm,    // LOADI + integer immediate 0 (stub).
  Copy,        // MOV / pass arg[0] register to dest.
  Trap,        // Emit TRAP; no result register.
  Nop,         // Emit nothing (async stub).
  Discard,     // Evaluate arg[0], discard result.
  Custom,      // Delegate to a named emit_<canonical> method in IRGen.
};

// Sentinel: used for ir_class values that don't need a primary_opcode.
inline constexpr tisc::ir::Opcode kNoOpcode = tisc::ir::Opcode::NOP;

// Sentinel arity: unchecked (no current builtins use this).
inline constexpr int8_t kArityAny = -1;

// ---------------------------------------------------------------------------
// BuiltinDef — one row per stdlib function (or alias).
// ---------------------------------------------------------------------------
struct BuiltinDef {
  // Dotted stdlib surface name as written in T81Lang source, e.g. "std.tensor.attention".
  // For bare canonicals (read_code etc.) this equals canonical.
  std::string_view stdlib_name;

  // Canonical internal name used in SA dispatch and IRGen, e.g. "Tensor.attention".
  std::string_view canonical;

  // Expected argument count.  kArityAny = unchecked.
  int8_t arity;

  // Static return type.  Kind::Unknown = polymorphic; SA must supply custom check.
  Type::Kind return_kind;

  // Non-empty only when return_kind == Kind::Custom.
  std::string_view return_custom_name;

  // Minimum @tier annotation required on the calling function.  nullopt = unrestricted.
  std::optional<int> min_tier;

  // Whether this call counts as an effect surface (blocked inside @pure functions
  // and Tier-1-or-lower contexts).
  bool is_effect_surface;

  // How IR is emitted.
  BuiltinIRClass ir_class;

  // Primary opcode (Direct / Constructor / Trap).  kNoOpcode otherwise.
  tisc::ir::Opcode primary_opcode;

  // String literal embedded in LOADI for LoadiSym / LoadiFloat classes.
  std::string_view ir_sym_literal;

  // true → SA visit(CallExpr) must still call a per-canonical dispatch block
  //        (polymorphic return, element-type extraction, literal-arg checks, …).
  // false → arity check + return_kind lookup is sufficient.
  bool needs_custom_sa_check;

  // true → IRGen visit(CallExpr) must call a per-canonical emit_XXX helper.
  // false → emit_table_driven_builtin() handles it from ir_class + primary_opcode.
  bool needs_custom_ir_emit;
};

// ---------------------------------------------------------------------------
// kBuiltinTable — the authoritative registry.
// ---------------------------------------------------------------------------
// clang-format off
inline constexpr BuiltinDef kBuiltinTable[] = {
  // ── Core ──────────────────────────────────────────────────────────────────
  {"std.core.assert",       "core_assert",       1, Type::Kind::Void,    {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, false, true  },
  {"std.core.debug",        "print",             1, Type::Kind::Void,    {},           {},       true,  BuiltinIRClass::Direct,      tisc::ir::Opcode::PRINT,      {}, false, false },
  {"std.core.unwrap_or",    "option_unwrap_or",  2, Type::Kind::Unknown, {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, true,  true  },

  // ── Option ────────────────────────────────────────────────────────────────
  {"std.option.is_some",    "option_is_some",    1, Type::Kind::Bool,    {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::OPTION_IS_SOME,  {}, false, false },
  {"std.option.is_none",    "option_is_none",    1, Type::Kind::Bool,    {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, false, true  },
  {"std.option.unwrap",     "option_unwrap",     1, Type::Kind::Unknown, {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, true,  true  },

  // ── Result ────────────────────────────────────────────────────────────────
  {"std.result.is_ok",      "result_is_ok",      1, Type::Kind::Bool,    {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::RESULT_IS_OK,    {}, false, false },
  {"std.result.is_err",     "result_is_err",     1, Type::Kind::Bool,    {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, false, true  },
  {"std.result.unwrap",     "result_unwrap",     1, Type::Kind::Unknown, {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, true,  true  },
  {"std.result.unwrap_err", "result_unwrap_err", 1, Type::Kind::Unknown, {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, true,  true  },

  // ── I/O ───────────────────────────────────────────────────────────────────
  {"std.io.println",        "print",             1, Type::Kind::Void,    {},           {},       true,  BuiltinIRClass::Direct,      tisc::ir::Opcode::PRINT,      {}, false, false },
  {"std.io.print_int",      "print",             1, Type::Kind::Void,    {},           {},       true,  BuiltinIRClass::Direct,      tisc::ir::Opcode::PRINT,      {}, false, false },
  {"std.io.print_float",    "print",             1, Type::Kind::Void,    {},           {},       true,  BuiltinIRClass::Direct,      tisc::ir::Opcode::PRINT,      {}, false, false },
  {"std.io.stream",         "io_stream",         0, Type::Kind::String,  {},           {2},      true,  BuiltinIRClass::LoadiSym,    kNoOpcode,                    "std.io.stream",   false, false },
  {"std.io.net",            "io_net",            0, Type::Kind::String,  {},           {2},      true,  BuiltinIRClass::LoadiSym,    kNoOpcode,                    "std.io.net",      false, false },

  // ── Math ──────────────────────────────────────────────────────────────────
  {"std.math.sin",          "sin",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FSIN,       {}, false, false },
  {"std.math.cos",          "cos",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FCOS,       {}, false, false },
  {"std.math.tan",          "tan",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FTAN,       {}, false, false },
  {"std.math.asin",         "asin",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FASIN,      {}, false, false },
  {"std.math.acos",         "acos",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FACOS,      {}, false, false },
  {"std.math.atan",         "atan",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FATAN,      {}, false, false },
  {"std.math.sinh",         "sinh",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FSINH,      {}, false, false },
  {"std.math.cosh",         "cosh",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FCOSH,      {}, false, false },
  {"std.math.tanh",         "tanh",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FTANH,      {}, false, false },
  {"std.math.exp",          "exp",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FEXP,       {}, false, false },
  {"std.math.log",          "log",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FLOG,       {}, false, false },
  {"std.math.sqrt",         "sqrt",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FSQRT,      {}, false, false },
  {"std.math.pow",          "pow",               2, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FPOW,       {}, false, false },
  {"std.math.clamp",        "clamp",             3, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, false, true  },
  {"std.math.abs",          "abs",               1, Type::Kind::Unknown, {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, true,  true  },

  // short unqualified aliases also accepted by SA (no std. prefix):
  {"sin",                   "sin",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FSIN,       {}, false, false },
  {"cos",                   "cos",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FCOS,       {}, false, false },
  {"tan",                   "tan",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FTAN,       {}, false, false },
  {"asin",                  "asin",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FASIN,      {}, false, false },
  {"acos",                  "acos",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FACOS,      {}, false, false },
  {"atan",                  "atan",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FATAN,      {}, false, false },
  {"sinh",                  "sinh",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FSINH,      {}, false, false },
  {"cosh",                  "cosh",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FCOSH,      {}, false, false },
  {"tanh",                  "tanh",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FTANH,      {}, false, false },
  {"exp",                   "exp",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FEXP,       {}, false, false },
  {"log",                   "log",               1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FLOG,       {}, false, false },
  {"sqrt",                  "sqrt",              1, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FSQRT,      {}, false, false },
  {"pow",                   "pow",               2, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Direct,      tisc::ir::Opcode::FPOW,       {}, false, false },
  {"clamp",                 "clamp",             3, Type::Kind::Float,   {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, false, true  },
  {"abs",                   "abs",               1, Type::Kind::Unknown, {},           {},       false, BuiltinIRClass::Custom,      kNoOpcode,                    {}, true,  true  },

  // BigInt
  {"std.math.bigint.from_int",  "bigint_from_int",  1, Type::Kind::BigInt,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::INT2BIGINT, {}, false, false },
  {"std.math.bigint.to_int",    "bigint_to_int",    1, Type::Kind::I32,      {}, {}, false, BuiltinIRClass::Custom,  kNoOpcode,                    {}, false, true  },
  {"std.math.bigint.add",       "bigint_add",       2, Type::Kind::BigInt,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::ADD,        {}, false, false },
  {"std.math.bigint.mul",       "bigint_mul",       2, Type::Kind::BigInt,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::MUL,        {}, false, false },

  // Fraction
  {"std.math.fraction.add",        "frac_add",       2, Type::Kind::Fraction, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::FRACADD,    {}, false, false },
  {"std.math.fraction.sub",        "frac_sub",       2, Type::Kind::Fraction, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::FRACSUB,    {}, false, false },
  {"std.math.fraction.mul",        "frac_mul",       2, Type::Kind::Fraction, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::FRACMUL,    {}, false, false },
  {"std.math.fraction.div",        "frac_div",       2, Type::Kind::Fraction, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::FRACDIV,    {}, false, false },
  {"std.math.fraction.from_int",   "frac_from_int",  1, Type::Kind::Fraction, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::I2FRAC,     {}, false, false },
  {"std.math.fraction.to_int",     "frac_to_int",    1, Type::Kind::I32,      {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::FRAC2I,     {}, false, false },
  {"std.math.fraction.from_float", "frac_from_float",1, Type::Kind::Fraction, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::F2FRAC,     {}, false, false },
  {"std.math.fraction.to_float",   "frac_to_float",  1, Type::Kind::Float,    {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::FRAC2F,     {}, false, false },

  // ── Sys / Async / Agent ───────────────────────────────────────────────────
  {"std.sys.exit",          "sys_exit",          1, Type::Kind::Void,    {},           {},       true,  BuiltinIRClass::Trap,        tisc::ir::Opcode::TRAP,       {}, false, false },
  {"std.sys.time",          "sys_time",          0, Type::Kind::Float,   {},           {},       true,  BuiltinIRClass::LoadiSym,    kNoOpcode,                    "0",               false, false },
  {"std.sys.entropy",       "sys_entropy",       0, Type::Kind::I32,     {},           {},       true,  BuiltinIRClass::LoadiImm,    kNoOpcode,                    {},                false, false },
  {"std.sys.proof",         "sys_proof",         0, Type::Kind::String,  {},           {},       true,  BuiltinIRClass::LoadiSym,    kNoOpcode,                    "std.sys.proof",   false, false },
  {"std.sys.reflect",       "sys_reflect",       0, Type::Kind::Void,    {},           {2},      true,  BuiltinIRClass::Direct,      tisc::ir::Opcode::META_REFLECT, {},              false, false },
  {"std.async.yield",       "async_yield",       0, Type::Kind::Void,    {},           {},       true,  BuiltinIRClass::Nop,         kNoOpcode,                    {}, false, false },
  {"std.async.sleep",       "async_sleep",       1, Type::Kind::Void,    {},           {},       true,  BuiltinIRClass::Discard,     kNoOpcode,                    {}, false, false },
  {"std.async.thread",      "async_thread",      0, Type::Kind::String,  {},           {2},      true,  BuiltinIRClass::LoadiSym,    kNoOpcode,                    "std.async.thread",  false, false },
  {"std.async.promise",     "async_promise",     0, Type::Kind::String,  {},           {2},      true,  BuiltinIRClass::LoadiSym,    kNoOpcode,                    "std.async.promise", false, false },
  {"std.agent.self_reflect","agent_self_reflect", 0, Type::Kind::Void,  {},            {2},      true,  BuiltinIRClass::Direct,      tisc::ir::Opcode::META_REFLECT, {},              false, false },

  // ── Symbolic / Polynomial ─────────────────────────────────────────────────
  {"std.symbolic.load",       "symbolic_load",        1, Type::Kind::Custom, "T81Symbolic",  {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::SYMLOAD,       {}, true,  false },
  {"std.symbolic.rewrite",    "symbolic_rewrite",     3, Type::Kind::Custom, "T81Symbolic",  {}, false, BuiltinIRClass::Custom,  kNoOpcode,                       {}, true,  true  },
  {"std.symbolic.canon",      "symbolic_canon",       1, Type::Kind::Custom, "T81Symbolic",  {}, false, BuiltinIRClass::Custom,  kNoOpcode,                       {}, true,  true  },
  {"std.symbolic.confluent",  "symbolic_confluent",   1, Type::Kind::Bool,   {},             {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::SYMCONFLUENCE, {}, true,  false },
  {"std.polynomial.load",     "polynomial_load",      1, Type::Kind::Custom, "T81Polynomial",{}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::SYMLOAD,       {}, true,  false },
  {"std.polynomial.rewrite",  "polynomial_rewrite",   3, Type::Kind::Custom, "T81Polynomial",{}, false, BuiltinIRClass::Custom,  kNoOpcode,                       {}, true,  true  },
  {"std.polynomial.canon",    "polynomial_canon",     1, Type::Kind::Custom, "T81Polynomial",{}, false, BuiltinIRClass::Custom,  kNoOpcode,                       {}, true,  true  },
  {"std.polynomial.confluent","polynomial_confluent", 1, Type::Kind::Bool,   {},             {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::SYMCONFLUENCE, {}, true,  false },
  // legacy unqualified names still accepted
  {"symbolic_confluent",      "symbolic_confluent",   1, Type::Kind::Bool,   {},             {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::SYMCONFLUENCE, {}, true,  false },
  {"polynomial_confluent",    "polynomial_confluent", 1, Type::Kind::Bool,   {},             {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::SYMCONFLUENCE, {}, true,  false },

  // ── Tensor / AI ───────────────────────────────────────────────────────────
  {"std.tensor.load",         "weights.load",       1, Type::Kind::I32,    {},  {2},   true,  BuiltinIRClass::Custom,  kNoOpcode,                  {}, true,  true  },
  {"std.tensor.from_list",    "Tensor.from_list",   1, Type::Kind::Tensor, {},  {},    false, BuiltinIRClass::Copy,    kNoOpcode,                  {}, false, false },
  {"std.tensor.matmul",       "Tensor.matmul",      2, Type::Kind::Tensor, {},  {2},   true,  BuiltinIRClass::Direct,  tisc::ir::Opcode::TMATMUL,  {}, false, false },
  {"std.tensor.vec_add",      "Tensor.vec_add",     2, Type::Kind::Tensor, {},  {},    false, BuiltinIRClass::Direct,  tisc::ir::Opcode::TVECADD,  {}, false, false },
  {"std.tensor.attention",    "Tensor.attention",   3, Type::Kind::Tensor, {},  {},    false, BuiltinIRClass::Custom,  kNoOpcode,                  {}, false, true  },
  {"std.tensor.qmatmul",      "Tensor.qmatmul",     3, Type::Kind::Tensor, {},  {},    false, BuiltinIRClass::Custom,  kNoOpcode,                  {}, false, true  },
  {"std.tensor.dot_product",  "tensor_dot",         2, Type::Kind::I32,    {},  {2},   true,  BuiltinIRClass::Custom,  kNoOpcode,                  {}, false, true  },

  // ── TNN — RFC-0037 Ternary Neural Network stdlib (lowers to RFC-0034 ops) ──
  // All TNN ops require @tier(2) or higher (inference context).
  {"std.tnn.matmul",  "tnn_matmul",  2, Type::Kind::Tensor, {}, {2}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.tnn.quant",   "tnn_quant",   2, Type::Kind::Tensor, {}, {2}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.tnn.attn",    "tnn_attn",    3, Type::Kind::Tensor, {}, {2}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.tnn.embed",   "tnn_embed",   2, Type::Kind::Tensor, {}, {2}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.tnn.accum",   "tnn_accum",   2, Type::Kind::Float,  {}, {2}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.tnn.act",     "tnn_act",     2, Type::Kind::BigInt, {}, {2}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },

  // ── Text ──────────────────────────────────────────────────────────────────
  {"std.text.str_len",        "str_len",         1, Type::Kind::I32,    {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRLEN,        {}, false, false },
  {"std.text.str_is_empty",   "str_is_empty",    1, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STREMPTY,      {}, false, false },
  {"std.text.concat",         "str_concat",      2, Type::Kind::String, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRCONCAT,     {}, false, false },
  {"std.text.starts_with",    "str_starts_with", 2, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRSTARTSWITH, {}, false, false },
  {"std.text.ends_with",      "str_ends_with",   2, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRENDSWITH,   {}, false, false },
  {"std.text.contains",       "str_contains",    2, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRCONTAINS,   {}, false, false },
  {"std.text.index_of",       "str_index_of",    2, Type::Kind::I32,    {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRINDEXOF,    {}, false, false },
  {"std.text.replace",        "str_replace",     3, Type::Kind::String, {}, {}, false, BuiltinIRClass::Custom,  kNoOpcode,                       {}, false, true  },
  {"std.text.to_string",      "str_to_string",   1, Type::Kind::String, {}, {}, false, BuiltinIRClass::Copy,    kNoOpcode,                       {}, false, false },
  {"std.text.from_bytes",     "str_to_string",   1, Type::Kind::String, {}, {}, false, BuiltinIRClass::Copy,    kNoOpcode,                       {}, false, false },
  {"std.text.split",          "str_split",       2, Type::Kind::Vector, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRSPLIT,      {}, true,  false },
  {"std.text.join",           "str_join",        2, Type::Kind::String, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRJOIN,       {}, true,  false },

  // ── Bytes ─────────────────────────────────────────────────────────────────
  {"std.bytes.len",           "bytes_len",         1, Type::Kind::I32,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRLEN,        {}, false, false },
  {"std.bytes.is_empty",      "bytes_is_empty",    1, Type::Kind::Bool,  {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STREMPTY,      {}, false, false },
  {"std.bytes.concat",        "bytes_concat",      2, Type::Kind::Bytes, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRCONCAT,     {}, false, false },
  {"std.bytes.starts_with",   "bytes_starts_with", 2, Type::Kind::Bool,  {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRSTARTSWITH, {}, false, false },
  {"std.bytes.ends_with",     "bytes_ends_with",   2, Type::Kind::Bool,  {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRENDSWITH,   {}, false, false },
  {"std.bytes.contains",      "bytes_contains",    2, Type::Kind::Bool,  {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRCONTAINS,   {}, false, false },
  {"std.bytes.index_of",      "bytes_index_of",    2, Type::Kind::I32,   {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRINDEXOF,    {}, false, false },
  {"std.bytes.replace",       "bytes_replace",     3, Type::Kind::Bytes, {}, {}, false, BuiltinIRClass::Custom,  kNoOpcode,                       {}, false, true  },
  {"std.bytes.split",         "bytes_split",       2, Type::Kind::Vector,{}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRSPLIT,      {}, true,  false },
  {"std.bytes.join",          "bytes_join",        2, Type::Kind::Bytes, {}, {}, false, BuiltinIRClass::Direct,  tisc::ir::Opcode::STRJOIN,       {}, true,  false },
  {"std.bytes.to_string",     "str_to_string",     1, Type::Kind::String,{}, {}, false, BuiltinIRClass::Copy,    kNoOpcode,                       {}, false, false },
  {"std.bytes.from_string",   "T81Bytes",          1, Type::Kind::Bytes, {}, {}, false, BuiltinIRClass::Copy,    kNoOpcode,                       {}, false, false },

  // ── Collections ───────────────────────────────────────────────────────────
  {"std.collections.len",          "collections_len",          1, Type::Kind::I32,    {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::VECLEN,    {}, false, false },
  {"std.collections.is_empty",     "collections_is_empty",     1, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::VECEMPTY,  {}, false, false },
  {"std.collections.first",        "collections_first",        1, Type::Kind::Unknown,{}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::VECFIRST,  {}, true,  false },
  {"std.collections.last",         "collections_last",         1, Type::Kind::Unknown,{}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::VECLAST,   {}, true,  false },
  {"std.collections.push",         "collections_push",         2, Type::Kind::Unknown,{}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::VECPUSH,   {}, true,  false },
  {"std.collections.pop",          "collections_pop",          1, Type::Kind::Unknown,{}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::VECPOP,    {}, true,  false },
  {"std.collections.list",         "collections_list",         0, Type::Kind::List,   {}, {}, false, BuiltinIRClass::Constructor, tisc::ir::Opcode::STRVECNEW, {}, false, false },
  {"std.collections.map",          "collections_map",          0, Type::Kind::Map,    {}, {}, false, BuiltinIRClass::Constructor, tisc::ir::Opcode::MapNew,    {}, false, false },
  {"std.collections.map_put",      "collections_map_put",      3, Type::Kind::Map,    {}, {}, false, BuiltinIRClass::Custom,      kNoOpcode,                   {}, false, true  },
  {"std.collections.map_get",      "collections_map_get",      2, Type::Kind::Unknown,{}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::MapGet,    {}, true,  false },
  {"std.collections.map_has",      "collections_map_has",      2, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::MapHas,    {}, false, false },
  {"std.collections.map_remove",   "collections_map_remove",   2, Type::Kind::Map,    {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::MapRemove, {}, false, false },
  {"std.collections.map_size",     "collections_map_size",     1, Type::Kind::I32,    {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::MapSize,   {}, false, false },
  {"std.collections.map_keys",     "collections_map_keys",     1, Type::Kind::Vector, {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::MapKeys,   {}, false, false },
  {"std.collections.set",          "collections_set",          0, Type::Kind::Set,    {}, {}, false, BuiltinIRClass::Constructor, tisc::ir::Opcode::SetNew,    {}, false, false },
  {"std.collections.set_size",     "collections_set_size",     1, Type::Kind::I32,    {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::SetSize,   {}, false, false },
  {"std.collections.set_has",      "collections_set_has",      2, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct,      tisc::ir::Opcode::SetHas,    {}, false, false },
  {"std.collections.set_add",      "collections_set_add",      2, Type::Kind::Set,    {}, {}, false, BuiltinIRClass::Custom,      kNoOpcode,                   {}, false, true  },
  {"std.collections.set_remove",   "collections_set_remove",   2, Type::Kind::Set,    {}, {}, false, BuiltinIRClass::Custom,      kNoOpcode,                   {}, false, true  },
  {"std.collections.tree",         "collections_tree",         0, Type::Kind::Vector, {}, {}, false, BuiltinIRClass::Constructor, tisc::ir::Opcode::STRVECNEW, {}, false, false },
  {"std.collections.graph",        "collections_graph",        0, Type::Kind::Vector, {}, {}, false, BuiltinIRClass::Constructor, tisc::ir::Opcode::STRVECNEW, {}, false, false },
  {"std.collections.graph_edge_count",  "collections_graph_edge_count",  1, Type::Kind::I32,   {}, {}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.collections.graph_has_edge",    "collections_graph_has_edge",    3, Type::Kind::Bool,  {}, {}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.collections.graph_add_edge",    "collections_graph_add_edge",    3, Type::Kind::Vector,{}, {}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.collections.graph_remove_edge", "collections_graph_remove_edge", 3, Type::Kind::Vector,{}, {}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.collections.graph_neighbors",   "collections_graph_neighbors",   2, Type::Kind::Vector,{}, {}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },
  {"std.collections.graph_canonical",   "collections_graph_canonical",   1, Type::Kind::String,{}, {}, false, BuiltinIRClass::Custom, kNoOpcode, {}, false, true },

  // ── Symbol ────────────────────────────────────────────────────────────────
  {"std.symbol.intern",     "symbol_intern",    1, Type::Kind::Symbol, {}, {}, false, BuiltinIRClass::Copy,   kNoOpcode,                  {}, false, false },
  {"std.symbol.to_string",  "symbol_to_string", 1, Type::Kind::String, {}, {}, false, BuiltinIRClass::Copy,   kNoOpcode,                  {}, false, false },
  {"std.symbol.eq",         "symbol_eq",        2, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct, tisc::ir::Opcode::CMP,      {}, false, false },
  {"std.symbol.ne",         "symbol_ne",        2, Type::Kind::Bool,   {}, {}, false, BuiltinIRClass::Direct, tisc::ir::Opcode::CMP,      {}, false, false },

  // ── Distributed (Tier 4) — previously SA-missing bug ──────────────────────
  {"std.distributed.gossip",    "dist_gossip",    1, Type::Kind::Void, {}, {4}, true, BuiltinIRClass::Direct,      tisc::ir::Opcode::GOSSIP,    {}, false, false },
  {"std.distributed.merge",     "dist_merge",     0, Type::Kind::I32,  {}, {4}, true, BuiltinIRClass::Constructor,  tisc::ir::Opcode::MERGE,     {}, false, false },
  {"std.distributed.sync",      "dist_sync",      1, Type::Kind::Void, {}, {4}, true, BuiltinIRClass::Direct,       tisc::ir::Opcode::TICKSYNC,  {}, false, false },
  {"std.distributed.coherence", "dist_coherence", 0, Type::Kind::I32,  {}, {4}, true, BuiltinIRClass::Constructor,  tisc::ir::Opcode::COHERENCE, {}, false, false },
  {"std.distributed.seal",      "dist_seal",      0, Type::Kind::I32,  {}, {4}, true, BuiltinIRClass::Constructor,  tisc::ir::Opcode::DISTSEAL,  {}, false, false },

  // ── Low-level / Meta (bare canonical names) ───────────────────────────────
  {"read_code",           "read_code",          1, Type::Kind::I32,  {}, {}, false, BuiltinIRClass::Direct, tisc::ir::Opcode::META_READ,    {}, false, false },
  {"write_code",          "write_code",         2, Type::Kind::Void, {}, {}, false, BuiltinIRClass::Direct, tisc::ir::Opcode::META_WRITE,   {}, false, false },
  {"refine",              "refine",             2, Type::Kind::I32,  {}, {}, false, BuiltinIRClass::Custom, kNoOpcode,                      {}, false, true  },
  {"observe_performance", "observe_performance",0, Type::Kind::I32,  {}, {}, false, BuiltinIRClass::LoadiImm, kNoOpcode,                   {}, false, false },
  {"optimize",            "optimize",           1, Type::Kind::I32,  {}, {}, false, BuiltinIRClass::Copy,   kNoOpcode,                      {}, false, false },
};
// clang-format on

// ---------------------------------------------------------------------------
// Lookup API — implemented in builtin_registry.cpp
// ---------------------------------------------------------------------------

// Returns the canonical name for a given stdlib surface name, or the input
// unchanged if it is not a known stdlib name.  Replaces the old
// canonical_stdlib_call_name() free function.
std::string_view canonical_name_for(std::string_view stdlib_name) noexcept;

// Lookup by stdlib surface name.  Returns nullptr for unknown names.
const BuiltinDef* lookup_builtin(std::string_view stdlib_name) noexcept;

// Lookup by canonical name.  Returns nullptr for unknown names.
// Multiple stdlib entries may share the same canonical; this returns the first.
const BuiltinDef* lookup_builtin_by_canonical(std::string_view canonical) noexcept;

}  // namespace t81::frontend
