#pragma once
#include <cstdint>

namespace t81::ir {

/**
 * @enum Opcode
 * @brief Canonical T81 IR opcodes, unifying TISC and legacy NewBook ISA.
 *
 * Values are stable; extend by appending to specific blocks.
 */
enum class Opcode : uint16_t {
  // --- Meta / Control (0x00xx) ---
  Nop = 0x0000,
  Halt = 0x0001,
  Jump = 0x0002,
  JumpIfZero = 0x0003,
  JumpIfNotZero = 0x0004,
  JumpIfNeg = 0x0005,
  JumpIfPos = 0x0006,
  Call = 0x0007,
  Ret = 0x0008,
  Trap = 0x0009,

  // --- Integer / Scalar ALU (0x01xx) ---
  Add = 0x0100,
  Sub = 0x0101,
  Mul = 0x0102,
  Div = 0x0103,
  Mod = 0x0104,
  Rem = 0x0105,
  And = 0x0106,
  Or = 0x0107,
  Xor = 0x0108,
  Not = 0x0109,
  Neg = 0x010A,
  Inc = 0x010B,
  Dec = 0x010C,
  Cmp = 0x010D,
  Move = 0x010E,
  LoadImm = 0x010F,

  // --- BigInt Ops (T243) (0x02xx) ---
  BigAdd = 0x0200,
  BigSub = 0x0201,
  BigMul = 0x0202,
  BigDiv = 0x0203,
  BigMod = 0x0204,
  BigCmp = 0x0205,

  // --- Tensor Ops (T729) (0x03xx) ---
  TDot = 0x0300,
  TTranspose = 0x0301,
  TSlice2D = 0x0302,
  TReshape = 0x0303,
  TMatMul = 0x0304,
  TReduce = 0x0305,
  TVecAdd = 0x0306,

  // --- Memory / Stack / IO (0x04xx) ---
  Load = 0x0400,
  Store = 0x0401,
  Push = 0x0402,
  Pop = 0x0403,
  StackAlloc = 0x0404,
  StackFree = 0x0405,
  HeapAlloc = 0x0406,
  HeapFree = 0x0407,

  // --- Axion / Capability / System (0x05xx) ---
  AxRead = 0x0500,
  AxSet = 0x0501,
  AxVerify = 0x0502,
  CapCheck = 0x0503,
  CapGrant = 0x0504,
  WeightsLoad = 0x0505,
  MetaRead = 0x0506,
  MetaWrite = 0x0507,
  MetaReflect = 0x0508,
  MetaRefine = 0x0509,
};

/**
 * @enum OpcodeFlags
 * @brief Metadata flags for IR opcodes.
 */
enum OpcodeFlags : uint32_t {
  OP_FLAG_NONE = 0,
  OP_FLAG_PRIVILEGED = 1 << 0,  ///< Requires Axion privileged context.
  OP_FLAG_BRANCH = 1 << 1,      ///< Instruction may change PC non-sequentially.
  OP_FLAG_TERMINATOR = 1 << 2,  ///< Ends a basic block.
  OP_FLAG_MEMORY = 1 << 3,      ///< Accesses memory (Load/Store).
};

/**
 * @struct OpcodeDesc
 * @brief Metadata description for an opcode.
 */
struct OpcodeDesc {
  Opcode op;
  const char* name;
  uint32_t flags;
};

inline OpcodeDesc get_opcode_desc(Opcode op) {
  switch (op) {
    // --- Meta / Control ---
    case Opcode::Nop:
      return {op, "nop", OP_FLAG_NONE};
    case Opcode::Halt:
      return {op, "halt", OP_FLAG_TERMINATOR};
    case Opcode::Jump:
      return {op, "jump", OP_FLAG_BRANCH | OP_FLAG_TERMINATOR};
    case Opcode::JumpIfZero:
      return {op, "jz", OP_FLAG_BRANCH};
    case Opcode::JumpIfNotZero:
      return {op, "jnz", OP_FLAG_BRANCH};
    case Opcode::JumpIfNeg:
      return {op, "jneg", OP_FLAG_BRANCH};
    case Opcode::JumpIfPos:
      return {op, "jpos", OP_FLAG_BRANCH};
    case Opcode::Call:
      return {op, "call", OP_FLAG_BRANCH};
    case Opcode::Ret:
      return {op, "ret", OP_FLAG_BRANCH | OP_FLAG_TERMINATOR};
    case Opcode::Trap:
      return {op, "trap", OP_FLAG_TERMINATOR};

    // --- Integer / Scalar ALU ---
    case Opcode::Add:
      return {op, "add", OP_FLAG_NONE};
    case Opcode::Sub:
      return {op, "sub", OP_FLAG_NONE};
    case Opcode::Mul:
      return {op, "mul", OP_FLAG_NONE};
    case Opcode::Div:
      return {op, "div", OP_FLAG_NONE};
    case Opcode::Mod:
      return {op, "mod", OP_FLAG_NONE};
    case Opcode::Rem:
      return {op, "rem", OP_FLAG_NONE};
    case Opcode::And:
      return {op, "and", OP_FLAG_NONE};
    case Opcode::Or:
      return {op, "or", OP_FLAG_NONE};
    case Opcode::Xor:
      return {op, "xor", OP_FLAG_NONE};
    case Opcode::Not:
      return {op, "not", OP_FLAG_NONE};
    case Opcode::Neg:
      return {op, "neg", OP_FLAG_NONE};
    case Opcode::Inc:
      return {op, "inc", OP_FLAG_NONE};
    case Opcode::Dec:
      return {op, "dec", OP_FLAG_NONE};
    case Opcode::Cmp:
      return {op, "cmp", OP_FLAG_NONE};
    case Opcode::Move:
      return {op, "move", OP_FLAG_NONE};
    case Opcode::LoadImm:
      return {op, "load_imm", OP_FLAG_NONE};

    // --- BigInt Ops ---
    case Opcode::BigAdd:
      return {op, "big_add", OP_FLAG_NONE};
    case Opcode::BigSub:
      return {op, "big_sub", OP_FLAG_NONE};
    case Opcode::BigMul:
      return {op, "big_mul", OP_FLAG_NONE};
    case Opcode::BigDiv:
      return {op, "big_div", OP_FLAG_NONE};
    case Opcode::BigMod:
      return {op, "big_mod", OP_FLAG_NONE};
    case Opcode::BigCmp:
      return {op, "big_cmp", OP_FLAG_NONE};

    // --- Tensor Ops ---
    case Opcode::TDot:
      return {op, "tdot", OP_FLAG_NONE};
    case Opcode::TTranspose:
      return {op, "ttranspose", OP_FLAG_NONE};
    case Opcode::TSlice2D:
      return {op, "tslice2d", OP_FLAG_NONE};
    case Opcode::TReshape:
      return {op, "treshape", OP_FLAG_NONE};
    case Opcode::TMatMul:
      return {op, "tmatmul", OP_FLAG_NONE};
    case Opcode::TReduce:
      return {op, "treduce", OP_FLAG_NONE};
    case Opcode::TVecAdd:
      return {op, "tvecadd", OP_FLAG_NONE};

    // --- AI-Native Inference Opcodes (RFC-0026) ---
    case Opcode::ATTN:
      return {op, "attn", OP_FLAG_NONE};
    case Opcode::QMATMUL:
      return {op, "qmatmul", OP_FLAG_NONE};
    case Opcode::WLOAD:
      return {op, "wload", OP_FLAG_PRIVILEGED};
    case Opcode::EMBED:
      return {op, "embed", OP_FLAG_NONE};
    case Opcode::GATHER:
      return {op, "gather", OP_FLAG_NONE};
    case Opcode::SCATTER:
      return {op, "scatter", OP_FLAG_NONE};

    // --- Memory / Stack / IO ---
    case Opcode::Load:
      return {op, "load", OP_FLAG_MEMORY};
    case Opcode::Store:
      return {op, "store", OP_FLAG_MEMORY};
    case Opcode::Push:
      return {op, "push", OP_FLAG_MEMORY};
    case Opcode::Pop:
      return {op, "pop", OP_FLAG_MEMORY};
    case Opcode::StackAlloc:
      return {op, "stack_alloc", OP_FLAG_MEMORY};
    case Opcode::StackFree:
      return {op, "stack_free", OP_FLAG_MEMORY};
    case Opcode::HeapAlloc:
      return {op, "heap_alloc", OP_FLAG_MEMORY};
    case Opcode::HeapFree:
      return {op, "heap_free", OP_FLAG_MEMORY};

    // --- Axion / System ---
    case Opcode::AxRead:
      return {op, "axread", OP_FLAG_PRIVILEGED};
    case Opcode::AxSet:
      return {op, "axset", OP_FLAG_PRIVILEGED};
    case Opcode::AxVerify:
      return {op, "axverify", OP_FLAG_PRIVILEGED};
    case Opcode::CapCheck:
      return {op, "capcheck", OP_FLAG_NONE};
    case Opcode::CapGrant:
      return {op, "capgrant", OP_FLAG_PRIVILEGED};
    case Opcode::WeightsLoad:
      return {op, "weights_load", OP_FLAG_PRIVILEGED};
    case Opcode::MetaRead:
      return {op, "metaread", OP_FLAG_PRIVILEGED};
    case Opcode::MetaWrite:
      return {op, "metawrite", OP_FLAG_PRIVILEGED};
    case Opcode::MetaReflect:
      return {op, "metareflect", OP_FLAG_PRIVILEGED};
    case Opcode::MetaRefine:
      return {op, "metarefine", OP_FLAG_PRIVILEGED};

    default:
      return {op, "unknown", OP_FLAG_NONE};
  }
}

}  // namespace t81::ir
