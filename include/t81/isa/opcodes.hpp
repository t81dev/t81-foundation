#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace t81::tisc {
// Opcode definitions per spec/tisc-spec.md (Chapters 3 & 44).
enum class Opcode : std::uint8_t {
  Nop = 0,
  Halt = 1,
  LoadImm = 2,
  Load = 3,
  Store = 4,
  Add = 5,
  Sub = 6,
  Mul = 7,
  Div = 8,
  Mod = 9,
  Jump = 10,
  JumpIfZero = 11,
  Mov = 12,
  Inc = 13,
  Dec = 14,
  Cmp = 15,
  Push = 16,
  Pop = 17,
  TNot = 18,
  TAnd = 19,
  TOr = 20,
  TXor = 21,
  AxRead = 22,
  AxSet = 23,
  AxVerify = 24,
  JumpIfNotZero = 25,
  Call = 26,
  Ret = 27,
  Trap = 28,
  I2F = 29,
  F2I = 30,
  I2Frac = 31,
  Frac2I = 32,
  TVecAdd = 33,
  TMatMul = 34,
  TTenDot = 35,
  FAdd = 36,
  FSub = 37,
  FMul = 38,
  FDiv = 39,
  FracAdd = 40,
  FracSub = 41,
  FracMul = 42,
  FracDiv = 43,
  SetF = 44,
  ChkShape = 45,
  MakeOptionSome = 46,
  MakeOptionNone = 47,
  MakeResultOk = 48,
  MakeResultErr = 49,
  OptionIsSome = 50,
  OptionUnwrap = 51,
  ResultIsOk = 52,
  ResultUnwrapOk = 53,
  ResultUnwrapErr = 54,
  MakeEnumVariant = 55,
  MakeEnumVariantPayload = 56,
  EnumIsVariant = 57,
  EnumUnwrapPayload = 58,
  Neg = 59,
  JumpIfNegative = 60,
  JumpIfPositive = 61,
  Less = 62,
  LessEqual = 63,
  Greater = 64,
  GreaterEqual = 65,
  Equal = 66,
  NotEqual = 67,
  StackAlloc,
  StackFree,
  HeapAlloc,
  HeapFree,
  WeightsLoad,
  TExp,
  TSqrt,
  TSiLU,
  TSoftmax,
  TRMSNorm,
  TRoPE,
  TVecMul,
  TTranspose,
  FSin,
  FCos,
  FTan,
  FAsin,
  FAcos,
  FAtan,
  FSinh,
  FCosh,
  FTanh,
  FSqrt,
  FExp,
  FLog,
  FPow,
  MetaRead,
  MetaWrite,
  MetaReflect,
  MetaRefine,
  Print,
  StrLen,
  StrEmpty,
  VecLen,
  VecEmpty,
  VecFirst,
  VecLast,
  VecPush,
  VecPop,
  TGet,
  TNew,
  TSet,
  StrConcat,
  StrStartsWith,
  StrEndsWith,
  StrContains,
  StrIndexOf,
  StrReplace,
  StrVecNew,
  StrVecPush,
  StrSplit,
  StrJoin,
  MakeComplex,
  TLoadHash,
  TID,
  NSend,
  NRecv,
  VWait,
  VYield,
  // New Opcodes from Spec Chapter 44
  TNorm,
  Canon,
  MemZero,
  Copy,
  AxHalt,
  Assert,
  SymLoad,
  SymRewrite,
  SymConfluence,
  SymCanon,
  SymBind,
  ReflCap,
  ReflJustify,
  ReflCheck,
  ReflTrace,
  ReflSeal,
  Recurse,
  Contract,
  Entropy,
  Depth,
  Terminate,
  Merge,
  Gossip,
  TickSync,
  Coherence,
  DistSeal,
  InfSeed,
  InfExpand,
  InfCollapse,
  InfConverge,
  InfSignature,
  AxCheck,
  AxSign,
  AxLineage,
  AxCanon,
  AxReport,
  F2Frac,
  Frac2F,
  TNeuralFwd,
  TNeuralBwd,
  BitAnd,
  BitOr,
  BitXor,
  BitNot,
  BitShl,
  BitShr,
  BitUShr,
  // Map/Set Scaffolding Opcodes
  MapNew,
  MapPut,
  MapGet,
  MapHas,
  MapRemove,
  MapKeys,
  MapSize,
  SetNew,
  SetAdd,
  SetRemove,
  SetHas,
  SetSize,
  TShape,  // Get tensor shape dimension: A=Dest, B=Tens, C=DimIdx
  // RFC-0026 phase-1 AI opcode subset (dispatch stubs initially fail-closed).
  ATTN,
  QMATMUL,
  EMBED,
};

[[nodiscard]] constexpr std::string_view opcode_name(Opcode opcode) {
  switch (opcode) {
    case Opcode::Nop:
      return "Nop";
    case Opcode::Halt:
      return "Halt";
    case Opcode::LoadImm:
      return "LoadImm";
    case Opcode::Load:
      return "Load";
    case Opcode::Store:
      return "Store";
    case Opcode::Add:
      return "Add";
    case Opcode::Sub:
      return "Sub";
    case Opcode::Mul:
      return "Mul";
    case Opcode::Div:
      return "Div";
    case Opcode::Mod:
      return "Mod";
    case Opcode::Jump:
      return "Jump";
    case Opcode::JumpIfZero:
      return "JumpIfZero";
    case Opcode::Mov:
      return "Mov";
    case Opcode::Inc:
      return "Inc";
    case Opcode::Dec:
      return "Dec";
    case Opcode::Cmp:
      return "Cmp";
    case Opcode::Push:
      return "Push";
    case Opcode::Pop:
      return "Pop";
    case Opcode::TNot:
      return "TNot";
    case Opcode::TAnd:
      return "TAnd";
    case Opcode::TOr:
      return "TOr";
    case Opcode::TXor:
      return "TXor";
    case Opcode::AxRead:
      return "AxRead";
    case Opcode::AxSet:
      return "AxSet";
    case Opcode::AxVerify:
      return "AxVerify";
    case Opcode::JumpIfNotZero:
      return "JumpIfNotZero";
    case Opcode::Call:
      return "Call";
    case Opcode::Ret:
      return "Ret";
    case Opcode::Trap:
      return "Trap";
    case Opcode::I2F:
      return "I2F";
    case Opcode::F2I:
      return "F2I";
    case Opcode::I2Frac:
      return "I2Frac";
    case Opcode::Frac2I:
      return "Frac2I";
    case Opcode::TVecAdd:
      return "TVecAdd";
    case Opcode::TMatMul:
      return "TMatMul";
    case Opcode::TTenDot:
      return "TTenDot";
    case Opcode::FAdd:
      return "FAdd";
    case Opcode::FSub:
      return "FSub";
    case Opcode::FMul:
      return "FMul";
    case Opcode::FDiv:
      return "FDiv";
    case Opcode::FracAdd:
      return "FracAdd";
    case Opcode::FracSub:
      return "FracSub";
    case Opcode::FracMul:
      return "FracMul";
    case Opcode::FracDiv:
      return "FracDiv";
    case Opcode::SetF:
      return "SetF";
    case Opcode::ChkShape:
      return "ChkShape";
    case Opcode::MakeOptionSome:
      return "MakeOptionSome";
    case Opcode::MakeOptionNone:
      return "MakeOptionNone";
    case Opcode::MakeResultOk:
      return "MakeResultOk";
    case Opcode::MakeResultErr:
      return "MakeResultErr";
    case Opcode::OptionIsSome:
      return "OptionIsSome";
    case Opcode::OptionUnwrap:
      return "OptionUnwrap";
    case Opcode::ResultIsOk:
      return "ResultIsOk";
    case Opcode::ResultUnwrapOk:
      return "ResultUnwrapOk";
    case Opcode::ResultUnwrapErr:
      return "ResultUnwrapErr";
    case Opcode::MakeEnumVariant:
      return "MakeEnumVariant";
    case Opcode::MakeEnumVariantPayload:
      return "MakeEnumVariantPayload";
    case Opcode::EnumIsVariant:
      return "EnumIsVariant";
    case Opcode::EnumUnwrapPayload:
      return "EnumUnwrapPayload";
    case Opcode::Neg:
      return "Neg";
    case Opcode::JumpIfNegative:
      return "JumpIfNegative";
    case Opcode::JumpIfPositive:
      return "JumpIfPositive";
    case Opcode::Less:
      return "Less";
    case Opcode::LessEqual:
      return "LessEqual";
    case Opcode::Greater:
      return "Greater";
    case Opcode::GreaterEqual:
      return "GreaterEqual";
    case Opcode::Equal:
      return "Equal";
    case Opcode::NotEqual:
      return "NotEqual";
    case Opcode::StackAlloc:
      return "StackAlloc";
    case Opcode::StackFree:
      return "StackFree";
    case Opcode::HeapAlloc:
      return "HeapAlloc";
    case Opcode::HeapFree:
      return "HeapFree";
    case Opcode::WeightsLoad:
      return "WeightsLoad";
    case Opcode::TExp:
      return "TExp";
    case Opcode::TSqrt:
      return "TSqrt";
    case Opcode::TSiLU:
      return "TSiLU";
    case Opcode::TSoftmax:
      return "TSoftmax";
    case Opcode::TRMSNorm:
      return "TRMSNorm";
    case Opcode::TRoPE:
      return "TRoPE";
    case Opcode::TVecMul:
      return "TVecMul";
    case Opcode::TTranspose:
      return "TTranspose";
    case Opcode::FSin:
      return "FSin";
    case Opcode::FCos:
      return "FCos";
    case Opcode::FTan:
      return "FTan";
    case Opcode::FAsin:
      return "FAsin";
    case Opcode::FAcos:
      return "FAcos";
    case Opcode::FAtan:
      return "FAtan";
    case Opcode::FSinh:
      return "FSinh";
    case Opcode::FCosh:
      return "FCosh";
    case Opcode::FTanh:
      return "FTanh";
    case Opcode::FSqrt:
      return "FSqrt";
    case Opcode::FExp:
      return "FExp";
    case Opcode::FLog:
      return "FLog";
    case Opcode::FPow:
      return "FPow";
    case Opcode::MetaRead:
      return "MetaRead";
    case Opcode::MetaWrite:
      return "MetaWrite";
    case Opcode::MetaReflect:
      return "MetaReflect";
    case Opcode::MetaRefine:
      return "MetaRefine";
    case Opcode::Print:
      return "Print";
    case Opcode::StrLen:
      return "StrLen";
    case Opcode::StrEmpty:
      return "StrEmpty";
    case Opcode::VecLen:
      return "VecLen";
    case Opcode::VecEmpty:
      return "VecEmpty";
    case Opcode::VecFirst:
      return "VecFirst";
    case Opcode::VecLast:
      return "VecLast";
    case Opcode::VecPush:
      return "VecPush";
    case Opcode::VecPop:
      return "VecPop";
    case Opcode::TGet:
      return "TGet";
    case Opcode::TNew:
      return "TNew";
    case Opcode::TSet:
      return "TSet";
    case Opcode::StrConcat:
      return "StrConcat";
    case Opcode::StrStartsWith:
      return "StrStartsWith";
    case Opcode::StrEndsWith:
      return "StrEndsWith";
    case Opcode::StrContains:
      return "StrContains";
    case Opcode::StrIndexOf:
      return "StrIndexOf";
    case Opcode::StrReplace:
      return "StrReplace";
    case Opcode::StrVecNew:
      return "StrVecNew";
    case Opcode::StrVecPush:
      return "StrVecPush";
    case Opcode::StrSplit:
      return "StrSplit";
    case Opcode::StrJoin:
      return "StrJoin";
    case Opcode::MakeComplex:
      return "MakeComplex";
    case Opcode::TLoadHash:
      return "TLoadHash";
    case Opcode::TID:
      return "TID";
    case Opcode::NSend:
      return "NSend";
    case Opcode::NRecv:
      return "NRecv";
    case Opcode::VWait:
      return "VWait";
    case Opcode::VYield:
      return "VYield";
    case Opcode::TNorm:
      return "TNorm";
    case Opcode::Canon:
      return "Canon";
    case Opcode::MemZero:
      return "MemZero";
    case Opcode::Copy:
      return "Copy";
    case Opcode::AxHalt:
      return "AxHalt";
    case Opcode::Assert:
      return "Assert";
    case Opcode::SymLoad:
      return "SymLoad";
    case Opcode::SymRewrite:
      return "SymRewrite";
    case Opcode::SymConfluence:
      return "SymConfluence";
    case Opcode::SymCanon:
      return "SymCanon";
    case Opcode::SymBind:
      return "SymBind";
    case Opcode::ReflCap:
      return "ReflCap";
    case Opcode::ReflJustify:
      return "ReflJustify";
    case Opcode::ReflCheck:
      return "ReflCheck";
    case Opcode::ReflTrace:
      return "ReflTrace";
    case Opcode::ReflSeal:
      return "ReflSeal";
    case Opcode::Recurse:
      return "Recurse";
    case Opcode::Contract:
      return "Contract";
    case Opcode::Entropy:
      return "Entropy";
    case Opcode::Depth:
      return "Depth";
    case Opcode::Terminate:
      return "Terminate";
    case Opcode::Merge:
      return "Merge";
    case Opcode::Gossip:
      return "Gossip";
    case Opcode::TickSync:
      return "TickSync";
    case Opcode::Coherence:
      return "Coherence";
    case Opcode::DistSeal:
      return "DistSeal";
    case Opcode::InfSeed:
      return "InfSeed";
    case Opcode::InfExpand:
      return "InfExpand";
    case Opcode::InfCollapse:
      return "InfCollapse";
    case Opcode::InfConverge:
      return "InfConverge";
    case Opcode::InfSignature:
      return "InfSignature";
    case Opcode::AxCheck:
      return "AxCheck";
    case Opcode::AxSign:
      return "AxSign";
    case Opcode::AxLineage:
      return "AxLineage";
    case Opcode::AxCanon:
      return "AxCanon";
    case Opcode::AxReport:
      return "AxReport";
    case Opcode::F2Frac:
      return "F2Frac";
    case Opcode::Frac2F:
      return "Frac2F";
    case Opcode::TNeuralFwd:
      return "TNeuralFwd";
    case Opcode::TNeuralBwd:
      return "TNeuralBwd";
    case Opcode::BitAnd:
      return "BitAnd";
    case Opcode::BitOr:
      return "BitOr";
    case Opcode::BitXor:
      return "BitXor";
    case Opcode::BitNot:
      return "BitNot";
    case Opcode::BitShl:
      return "BitShl";
    case Opcode::BitShr:
      return "BitShr";
    case Opcode::BitUShr:
      return "BitUShr";
    case Opcode::MapNew:
      return "MapNew";
    case Opcode::MapPut:
      return "MapPut";
    case Opcode::MapGet:
      return "MapGet";
    case Opcode::MapHas:
      return "MapHas";
    case Opcode::MapRemove:
      return "MapRemove";
    case Opcode::MapKeys:
      return "MapKeys";
    case Opcode::MapSize:
      return "MapSize";
    case Opcode::SetNew:
      return "SetNew";
    case Opcode::SetAdd:
      return "SetAdd";
    case Opcode::SetRemove:
      return "SetRemove";
    case Opcode::SetHas:
      return "SetHas";
    case Opcode::SetSize:
      return "SetSize";
    case Opcode::TShape:
      return "TShape";
    case Opcode::ATTN:
      return "ATTN";
    case Opcode::QMATMUL:
      return "QMATMUL";
    case Opcode::EMBED:
      return "EMBED";
  }
  return "Unknown";
}

inline constexpr std::array<Opcode, static_cast<std::size_t>(Opcode::EMBED) + 1> kAllOpcodes = [] {
  std::array<Opcode, static_cast<std::size_t>(Opcode::EMBED) + 1> values{};
  for (std::size_t i = 0; i < values.size(); ++i) {
    values[i] = static_cast<Opcode>(i);
  }
  return values;
}();

[[nodiscard]] constexpr bool is_valid_opcode(std::uint8_t raw_opcode) {
  return raw_opcode <= static_cast<std::uint8_t>(Opcode::EMBED);
}
}  // namespace t81::tisc
