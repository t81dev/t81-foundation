#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "t81/axion/api.hpp"
#include "t81/axion/engine.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"

int main() {
  auto expect = [](bool cond, const char* msg) -> bool {
    if (!cond) {
      std::cerr << "axion_stub_test failure: " << msg << "\n";
      return false;
    }
    return true;
  };

  using namespace t81::axion;

  // Version and runtime name are updated in the façade.
  [[maybe_unused]] auto v = AxionContext::runtime_version();
  if (!expect(v.major == 1 && v.minor == 2 && v.patch == 0, "runtime version mismatch")) return 1;
  if (!expect(std::string(AxionContext::runtime_name()) == "Axion-Façade", "runtime name mismatch"))
    return 1;

  [[maybe_unused]] AxionContext cx;
  cx.reset_telemetry();

  // Test capabilities
  [[maybe_unused]] auto caps = cx.capabilities();
  if (!expect(!caps.empty(), "capabilities unexpectedly empty")) return 1;
  if (!expect(std::string(caps[0].name) == "DeterministicExecution", "first capability mismatch"))
    return 1;

  // Prepare a request
  Signal sig{};
  sig.kind = 0x42;
  sig.flags = 0xA5A5;
  sig.nonce = 0x1122334455667788ull;

  [[maybe_unused]] Buffer in;
  const std::string payload = "ping-axion";
  in.data.assign(payload.begin(), payload.end());

  [[maybe_unused]] Buffer out;
  [[maybe_unused]] auto st = cx.submit(sig, in, out);
  if (!expect(st == Status::Ok, "submit returned non-OK")) return 1;

  // Response must contain input + trailer ("AXN\2" + fields LE)
  if (!expect(out.data.size() >= in.data.size() + 4 + 4 + 4, "response too short")) return 1;
  // Trailer magic
  [[maybe_unused]] size_t off = payload.size();
  if (!expect(out.data[off + 0] == 'A', "trailer[0] mismatch")) return 1;
  if (!expect(out.data[off + 1] == 'X', "trailer[1] mismatch")) return 1;
  if (!expect(out.data[off + 2] == 'N', "trailer[2] mismatch")) return 1;
  if (!expect(out.data[off + 3] == 0x02, "trailer version mismatch")) return 1;  // new version

  // Simple telemetry checks
  const auto& tele = cx.telemetry();
  if (!expect(tele.requests == 1, "telemetry requests mismatch")) return 1;
  if (!expect(tele.bytes_in == payload.size(), "telemetry bytes_in mismatch")) return 1;
  if (!expect(tele.bytes_out == out.data.size(), "telemetry bytes_out mismatch")) return 1;

  // Deterministic decision API test
  SyscallContext sctx{};
  sctx.instruction_count = 100;

  // Create context with a real policy engine that denies after 50 instructions
  [[maybe_unused]] auto policy_res = parse_policy("(policy (max-instructions 50))");
  if (!expect(policy_res.has_value(), "policy parse failed")) return 1;
  AxionContext cx_denied(make_policy_engine(std::move(policy_res.value())));

  [[maybe_unused]] auto verdict = cx_denied.evaluate(sctx);
  if (!expect(verdict.kind == VerdictKind::Deny, "expected deny verdict at count=100")) return 1;
  if (!expect(cx_denied.telemetry().denies == 1, "telemetry deny counter mismatch")) return 1;

  sctx.instruction_count = 10;
  [[maybe_unused]] auto verdict_allow = cx_denied.evaluate(sctx);
  if (!expect(verdict_allow.kind == VerdictKind::Allow, "expected allow verdict at count=10"))
    return 1;

  // Overflow traps must still throw when stderr logging is suppressed.
#if defined(_WIN32)
  _putenv_s("T81_AXION_TRAP_STDERR", "0");
#else
  setenv("T81_AXION_TRAP_STDERR", "0", 1);
#endif
  bool threw = false;
  try {
    trap_overflow("test-overflow");
  } catch (const std::overflow_error&) {
    threw = true;
  }
  if (!expect(threw, "trap_overflow did not throw")) return 1;

  std::cout << "axion_façade tests ok\n";
  return 0;
}
