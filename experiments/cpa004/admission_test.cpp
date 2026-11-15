#include <cstdio>
#include <cstring>
#include <string>
#include "cpa004_evaluator.hpp"
#include "cpa004_program.hpp"

static std::string output;
static void emit(const char* text) { output += text; }
static bool expect(bool condition, const char* message) {
  if (!condition) std::fprintf(stderr, "CPA004 admission check failed: %s\n", message);
  return condition;
}
int main() {
  unsigned char modified[sizeof(kCpa004Program)];
  if (!expect(cpa004_evaluate(nullptr, 351, emit) == Cpa004Malformed, "null input") ||
      !expect(cpa004_evaluate(kCpa004Program, 350, emit) == Cpa004Malformed, "truncated record") ||
      !expect(cpa004_evaluate(kCpa004Program, 351, nullptr) == Cpa004Malformed, "null sink") ||
      !expect(cpa004_evaluate(kCpa004Program, 351, emit, 65) == Cpa004OutsideProfile,
              "unbounded limit"))
    return 1;
  std::memcpy(modified, kCpa004Program, sizeof(modified));
  modified[0] = 0xff;
  if (!expect(cpa004_evaluate(modified, 351, emit) == Cpa004Unsupported, "undefined opcode"))
    return 1;
  modified[0] = 36;
  if (!expect(cpa004_evaluate(modified, 351, emit) == Cpa004Unsupported,
              "excluded valid FAdd opcode"))
    return 1;
  std::memcpy(modified, kCpa004Program, sizeof(modified));
  modified[1] = 81;
  if (!expect(cpa004_evaluate(modified, 351, emit) == Cpa004Malformed, "extended destination"))
    return 1;
  std::memcpy(modified, kCpa004Program, sizeof(modified));
  modified[5] = 0xfe;
  if (!expect(cpa004_evaluate(modified, 351, emit) == Cpa004ArtifactMismatch,
              "changed admitted instruction") ||
      !expect(output.empty(), "admission rejection emitted no execution evidence"))
    return 1;
  if (!expect(cpa004_evaluate(kCpa004Program, 351, emit, 1) == Cpa004InstructionLimit,
              "step exhaustion is not Halt") ||
      !expect(output.find("\"status\":\"instruction_limit\"") != std::string::npos,
              "explicit limit record"))
    return 1;
  output.clear();
  if (!expect(cpa004_evaluate(kCpa004Program, 351, emit) == Cpa004Halted, "positive artifact") ||
      !expect(output.find("\"sequence\":24,\"pc_before\":26") != std::string::npos,
              "24 actual steps") ||
      !expect(output.find("\"pc\":27,\"sp\":283,\"halted\":true") != std::string::npos,
              "positive Halt"))
    return 1;
  std::puts("CPA004 admission and limit checks passed (also active under NDEBUG)");
}
