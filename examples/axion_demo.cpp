#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "t81/t81.hpp"

int main() {
  using namespace t81::axion;

  std::cout << "[Axion Demo]\n";
  std::cout << "runtime: " << Context::runtime_name()
            << " v" << Context::runtime_version().str() << "\n";

  Context cx;

  // Build a request
  Signal sig{};
  sig.kind  = 0x1001;          // pretend op id
  sig.flags = 0x3;             // pretend flags
  sig.nonce = 0xDEADBEEFCAFEBABEull;

  Buffer in;
  const std::string payload = "hello-axion";
  in.data.assign(payload.begin(), payload.end());

  Buffer out;
  auto st = cx.submit(sig, in, out);
  if (st != Status::Ok) {
    std::cerr << "submit failed: " << static_cast<int>(st) << "\n";
    return 1;
  }

  // Dump response bytes (ASCII-friendly)
  std::cout << "response (" << out.data.size() << " bytes): ";
  for (uint8_t b : out.data) std::cout << (char)((b >= 32 && b < 127) ? b : '.');
  std::cout << "\n";

  // Telemetry
  const auto& tele = cx.telemetry();
  std::cout << "telemetry: "
            << "requests=" << tele.requests
            << " bytes_in=" << tele.bytes_in
            << " bytes_out=" << tele.bytes_out
            << " last_ms=" << tele.last_ms
            << "\n";

  return 0;
}
