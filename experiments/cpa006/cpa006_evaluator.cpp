// CPA-006: independent, bounded execution of the accepted hosted raw13 profile.
// Only freestanding integer types are imported. No t81::vm types or helpers.
#include "cpa006_evaluator.hpp"
#include "cpa006_program.hpp"  // Generated from the frozen raw binary by CMake.

namespace {
constexpr unsigned kInstructions = sizeof(kCpa006Program)/13, kWords = kInstructions+1536, kRegisters = 81;
constexpr const char* kDigest = kCpa006Digest;
enum Op : uint8_t {
  Halt = 1,
  LoadImm = 2,
  Load = 3,
  Store = 4,
  Add = 5,
  Mul = 7,
  Div = 8,
  Mod = 9,
  Jz = 11,
  Cmp = 15,
  TNot = 18,
  TAnd = 19,
  TOr = 20,
  TXor = 21,
  Jnz = 25,
  Trap = 28,
  Jn = 60
};
struct Instruction {
  uint8_t op;
  int32_t a, b, c;
};
struct Machine {
  int64_t registers[kRegisters], memory[kWords];
  uint8_t tags[kRegisters], memory_tags[kWords];
  unsigned pc, sp, tick;
  bool zero, negative, positive, halted;
};
static Machine state;
static const char* last_trap;
static_assert(sizeof(kCpa006Program) <= 64*13, "sealed suite program bound");
static_assert(sizeof(Machine) + sizeof(kCpa006Program) + 1024 < 32768,
              "fixed workspace plus formatter/decoder headroom");

int32_t signed_le32(const uint8_t* p) noexcept {
  const uint32_t u =
      uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
  // Avoid implementation-defined unsigned-to-signed narrowing.
  return u <= 0x7fffffffU ? static_cast<int32_t>(u) : -1 - static_cast<int32_t>(0xffffffffU - u);
}
Instruction decode(const uint8_t* bytes, unsigned pc) noexcept {
  const uint8_t* p = bytes + pc * 13;
  return {p[0], signed_le32(p + 1), signed_le32(p + 5), signed_le32(p + 9)};
}
const char* name(uint8_t op) noexcept {
  switch (op) {
    case Halt:
      return "Halt";
    case LoadImm:
      return "LoadImm";
    case Load:
      return "Load";
    case Store:
      return "Store";
    case Add:
      return "Add";
    case Mul:
      return "Mul";
    case Div:
      return "Div";
    case Mod:
      return "Mod";
    case Jz:
      return "JumpIfZero";
    case Cmp:
      return "Cmp";
    case TNot:
      return "TNot";
    case TAnd:
      return "TAnd";
    case TOr:
      return "TOr";
    case TXor:
      return "TXor";
    case Jnz:
      return "JumpIfNotZero";
    case Trap:
      return "Trap";
    case Jn:
      return "JumpIfNegative";
    default:
      return nullptr;
  }
}
uint64_t hash_word(uint64_t h, uint64_t word) noexcept {
  for (unsigned i = 0; i < 8; ++i) {
    h ^= (word >> (8 * i)) & 255U;
    h *= UINT64_C(1099511628211);
  }
  return h;
}
int64_t finish_hash(uint64_t h) noexcept {
  h &= UINT64_C(0x7fffffffffffffff);
  return h ? static_cast<int64_t>(h) : 1;
}
int64_t lineage(const uint8_t* bytes) noexcept {
  uint64_t h = hash_word(UINT64_C(1469598103934665603), kInstructions);
  for (unsigned pc = 0; pc < kInstructions; ++pc) {
    const Instruction i = decode(bytes, pc);
    h = hash_word(h, i.op);
    h = hash_word(h, static_cast<uint64_t>(int64_t(i.a)));
    h = hash_word(h, static_cast<uint64_t>(int64_t(i.b)));
    h = hash_word(h, static_cast<uint64_t>(int64_t(i.c)));
    h = hash_word(h, 0);
  }
  return finish_hash(h);
}
void sync(int64_t root) noexcept {
  state.registers[0] = 0;
  state.registers[75] = state.tick;
  state.registers[76] = root;
  uint64_t h = hash_word(UINT64_C(1469598103934665603), state.tick);
  h = hash_word(h, 0);
  h = hash_word(h, 0);
  h = hash_word(h, 0);
  h = hash_word(h, state.pc);
  h = hash_word(h, 0);  // fresh tier0, no recursion/contradiction
  state.registers[77] = finish_hash(h);
  state.registers[78] = 0;
  state.registers[79] = 0;
  state.registers[80] = state.halted ? 0 : 1;
}
void flags(int64_t value) noexcept {
  state.zero = value == 0;
  state.negative = value < 0;
  state.positive = value > 0;
}
void number(Cpa006Emit emit, int64_t value) noexcept {
  char buf[24];
  unsigned end = 23, pos = end;
  buf[end] = '\0';
  const bool neg = value < 0;
  uint64_t magnitude = neg ? uint64_t(-(value + 1)) + 1U : uint64_t(value);
  do {
    buf[--pos] = char('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude);
  if (neg) buf[--pos] = '-';
  emit(buf + pos);
}
void boolean(Cpa006Emit emit, bool b) noexcept { emit(b ? "true" : "false"); }
void quoted(Cpa006Emit emit, int64_t value) noexcept {
  emit("\"");
  number(emit, value);
  emit("\"");
}
void observation(Cpa006Emit emit, int before, const Instruction* insn, const char* status,
                 const char* trap = nullptr) noexcept {
  emit("CPA006_OBSERVATION={\"schema\":\"t81.cpa006.observation.v1\",\"program_sha256\":\"");
  emit(kDigest);
  emit("\",\"sequence\":");
  number(emit, state.tick);
  emit(",\"pc_before\":");
  if (before < 0)
    emit("null");
  else
    number(emit, before);
  emit(",\"instruction\":");
  if (!insn)
    emit("null");
  else {
    emit("{\"opcode\":");
    number(emit, insn->op);
    emit(",\"name\":\"");
    emit(name(insn->op));
    emit("\",\"a\":");
    number(emit, insn->a);
    emit(",\"b\":");
    number(emit, insn->b);
    emit(",\"c\":");
    number(emit, insn->c);
    emit("}");
  }
  emit(",\"status\":\"");
  emit(status);
  emit("\",\"trap\":");
  if (trap) {
    emit("\"");
    emit(trap);
    emit("\"");
  } else
    emit("null");
  emit(",\"state\":{\"pc\":");
  number(emit, state.pc);
  emit(",\"sp\":");
  number(emit, state.sp);
  emit(",\"halted\":");
  boolean(emit, state.halted);
  emit(",\"flags\":{\"zero\":");
  boolean(emit, state.zero);
  emit(",\"negative\":");
  boolean(emit, state.negative);
  emit(",\"positive\":");
  boolean(emit, state.positive);
  emit("},\"register_values\":[");
  for (unsigned r = 0; r < kRegisters; ++r) {
    if (r) emit(",");
    quoted(emit, state.registers[r]);
  }
  emit("],\"register_tags\":[");
  for (unsigned r = 0; r < kRegisters; ++r) {
    if (r) emit(",");
    number(emit, state.tags[r]);
  }
  emit("],\"memory\":{\"word_count\":");
  number(emit,kWords);
  emit(",\"default_value\":\"0\",\"default_tag\":0,\"overrides\":"
       "[");
  bool comma = false;
  for (unsigned a = 0; a < kWords; ++a)
    if (state.memory[a] || state.memory_tags[a]) {
      if (comma) emit(",");
      comma = true;
      emit("{\"address\":");
      number(emit, a);
      emit(",\"value\":");
      quoted(emit, state.memory[a]);
      emit(",\"tag\":");
      number(emit, state.memory_tags[a]);
      emit("}");
    }
  emit("]}}}\r\n");
}
bool reg(int32_t r) noexcept { return r >= 0 && r < int32_t(kRegisters); }
bool dest(int32_t r) noexcept { return reg(r); }
bool address(int32_t a) noexcept { return a >= int32_t(kInstructions) && a < int32_t(kWords); }
bool operands(const Instruction& i) noexcept {
  switch (i.op) {
    case Halt:
    case Trap:
      return true;
    case LoadImm:
      return dest(i.a);
    case Load:
      return dest(i.a);
    case Store:
      return reg(i.b);
    case TNot:
      return dest(i.a) && reg(i.b);
    case Cmp:
      return reg(i.a) && reg(i.b);
    case Jz:
    case Jnz:
      return i.a >= 0 && i.a <= int32_t(kInstructions) && reg(i.b);
    case Jn:
      return i.a >= 0 && i.a <= int32_t(kInstructions);
    default:
      return dest(i.a) && reg(i.b) && reg(i.c);
  }
}
}  // namespace

extern "C" int cpa006_evaluate(const uint8_t* bytes, unsigned length, Cpa006Emit emit,
                               unsigned limit) noexcept {
  if (!bytes || !emit || !length || length % 13 || length > 64*13) return Cpa006Malformed;
  if (limit > 64) return Cpa006OutsideProfile;
  // Reject unsupported/malformed input before any instruction or output side effect.
  for (unsigned pc = 0; pc < length/13; ++pc) {
    const Instruction i = decode(bytes, pc);
    if (!name(i.op)) return Cpa006Unsupported;

  }
  if (length != sizeof(kCpa006Program)) return Cpa006ArtifactMismatch;
  for (unsigned b = 0; b < length; ++b)
    if (bytes[b] != kCpa006Program[b]) return Cpa006ArtifactMismatch;
  for (unsigned r = 0; r < kRegisters; ++r) {
    state.registers[r] = 0;
    state.tags[r] = 0;
  }
  for (unsigned a = 0; a < kWords; ++a) {
    state.memory[a] = 0;
    state.memory_tags[a] = 0;
  }
  state.pc = 0;
  state.sp = kInstructions+256;
  state.tick = 0;
  state.zero = false;
  state.negative = false;
  state.positive = false;
  state.halted = false;
  const int64_t root = lineage(bytes);
  sync(root);
  observation(emit, -1, nullptr, "running");
  while (state.tick < limit && !state.halted) {
    if (state.pc >= kInstructions) return Cpa006OutsideProfile;
    const unsigned before = state.pc;
    const Instruction i = decode(bytes, state.pc++);
    ++state.tick;
    const int64_t lhs = reg(i.b) ? state.registers[i.b] : 0;
    const int64_t rhs = reg(i.c) ? state.registers[i.c] : 0;
    int64_t value = 0;
    bool write = false;
    const char* trap = nullptr;
    if (!operands(i)) trap = "DecodeFault";
    else switch (i.op) {
      case Halt:
        state.halted = true;
        break;
      case LoadImm:
        value = i.b;
        write = true;
        break;
      case Load:
        if (!address(i.b)) { trap="BoundsFault"; break; }
        value = state.memory[i.b];
        state.tags[i.a] = state.memory_tags[i.b];
        write = true;
        break;
      case Store:
        if (!address(i.a)) { trap="BoundsFault"; break; }
        state.memory[i.a] = lhs;
        state.memory_tags[i.a] = state.tags[i.b];
        break;
      case Add:
        if ((rhs > 0 && lhs > INT64_MAX - rhs) || (rhs < 0 && lhs < INT64_MIN - rhs))
          return Cpa006OutsideProfile;
        value = lhs + rhs;
        write = true;
        break;
      case Mul:
        if ((lhs > 0 &&
             ((rhs > 0 && lhs > INT64_MAX / rhs) || (rhs < 0 && rhs < INT64_MIN / lhs))) ||
            (lhs < 0 && ((rhs > 0 && lhs < INT64_MIN / rhs) || (rhs < 0 && lhs < INT64_MAX / rhs))))
          return Cpa006OutsideProfile;
        value = lhs * rhs;
        write = true;
        break;
      case Div:
      case Mod:
        if (!rhs) {
          trap = "DivisionFault";
          break;
        }
        if (lhs == (-INT64_C(9223372036854775807) - 1) && rhs == -1) return Cpa006OutsideProfile;
        value = i.op == Div ? lhs / rhs : lhs % rhs;
        write = true;
        break;
      case TNot:
      case TAnd:
      case TOr:
      case TXor:
        if (lhs < -1 || lhs > 1 || (i.op != TNot && (rhs < -1 || rhs > 1)))
          return Cpa006OutsideProfile;
        if (i.op == TNot)
          value = -lhs;
        else if (i.op == TAnd)
          value = lhs < rhs ? lhs : rhs;
        else if (i.op == TOr)
          value = lhs > rhs ? lhs : rhs;
        else {
          value = lhs - rhs;
          if (value > 1)
            value -= 3;
          else if (value < -1)
            value += 3;
        }
        write = true;
        break;
      case Cmp:
        flags(state.registers[i.a] > lhs ? 1 : (state.registers[i.a] < lhs ? -1 : 0));
        break;
      case Jz:
        if (lhs == 0) state.pc = unsigned(i.a);
        break;
      case Jnz:
        if (lhs != 0) state.pc = unsigned(i.a);
        break;
      case Jn:
        if (state.negative) state.pc = unsigned(i.a);
        break;
      case Trap:
        trap = "TrapInstruction";
        break;
      default:
        return Cpa006Unsupported;
    }
    if (write) {
      if (i.op == LoadImm && (i.a == 0 || (i.a >= 75 && i.a <= 80)))
        value = state.registers[i.a];
      else { state.registers[i.a] = value; state.tags[i.a] = 0; }
      flags(value);
    }
    sync(root);
    observation(emit, int(before), &i, trap ? "trap" : (state.halted ? "halted" : "running"), trap);
    if (trap) { last_trap=trap; return Cpa006Trap; }
  }
  if (!state.halted) {
    return Cpa006InstructionLimit;
  }
  return Cpa006Halted;
}

namespace {
void terminal(Cpa006Emit emit, int result, const char* reason) noexcept {
  emit("CPA006_RESULT={\"classification\":\"");
  emit(result==Cpa006Halted ? "halted" : result==Cpa006Trap ? "runtime_trap" :
       result==Cpa006InstructionLimit ? "instruction_limit" : "admission_rejection");
  emit("\",\"reason\":");
  if(reason) { emit("\""); emit(reason); emit("\""); } else emit("null");
  emit(",\"steps\":");
  number(emit,(result==Cpa006Halted || result==Cpa006Trap || result==Cpa006InstructionLimit) ? state.tick : 0);
  emit("}\r\n");
}
}
extern "C" void cpa006_run(Cpa006Emit emit) noexcept {
  emit("[cpa006] live post-ExitBootServices evaluator\r\nCPA006_ARTIFACT_HEX=");
  const char hex[] = "0123456789abcdef";
  for(unsigned n=0;n<sizeof(kCpa006Program);++n) {
    char s[3]={hex[kCpa006Program[n]>>4],hex[kCpa006Program[n]&15],0}; emit(s);
  }
  emit("\r\n");
  const int result=cpa006_evaluate(kCpa006Program,sizeof(kCpa006Program),emit,kCpa006Limit);
  const char* reason = result==Cpa006Malformed ? "malformed" :
                       result==Cpa006ArtifactMismatch ? "artifact_mismatch" :
                       result==Cpa006OutsideProfile ? "outside_profile" : nullptr;
  if(result==Cpa006Unsupported) {
    reason="unsupported_profile";
    for(unsigned p=0;p+13<=sizeof(kCpa006Program);p+=13)
      if(kCpa006Program[p]>=247 || (kCpa006Program[p]>=221 && kCpa006Program[p]<=223))
        reason="undefined_opcode";
  }
  if(result==Cpa006Trap) reason=last_trap;
  terminal(emit,result,reason);
  emit("CPA006_DONE\r\n");
}
