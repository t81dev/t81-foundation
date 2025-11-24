#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "t81/axion/api.hpp"
#include "t81/axion/policy.hpp"

int main() {
  using namespace t81::axion;

  // Version and runtime name are deterministic in the stub.
  auto v = Context::runtime_version();
  assert(v.major == 1 && v.minor == 1 && v.patch == 0);
  assert(std::string(Context::runtime_name()) == "Axion-Stub");

  Context cx;
  cx.reset_telemetry();

  // Prepare a request
  Signal sig{};
  sig.kind  = 0x42;
  sig.flags = 0xA5A5;
  sig.nonce = 0x1122334455667788ull;

  Buffer in;
  const std::string payload = "ping-axion";
  in.data.assign(payload.begin(), payload.end());

  Buffer out;
  auto st = cx.submit(sig, in, out);
  assert(st == Status::Ok);

  // Response must contain input + trailer ("AXN\1" + fields LE)
  assert(out.data.size() >= in.data.size() + 4 + 4 + 4 + 8);
  // Input echoed at start
  for (size_t i = 0; i < payload.size(); ++i) {
    assert(out.data[i] == static_cast<uint8_t>(payload[i]));
  }
  // Trailer magic
  size_t off = payload.size();
  assert(out.data[off+0] == 'A');
  assert(out.data[off+1] == 'X');
  assert(out.data[off+2] == 'N');
  assert(out.data[off+3] == 0x01);

  // Simple telemetry checks
  const auto& tele = cx.telemetry();
  assert(tele.requests == 1);
  assert(tele.bytes_in  == payload.size());
  assert(tele.bytes_out == out.data.size());
  assert(tele.last_ms >= 0.0);

  // Policy parsing smoke test
  auto policy = parse_policy("(policy (tier 3) (max-stack 59049))");
  assert(policy.has_value());
  assert(policy->tier == 3);
  assert(policy->max_stack.has_value());

  std::cout << "axion_stub ok\n";
  return 0;
}
