// Includes exact historical builders; the upstream test main is never called.
#define main audited_test_main_unused
#include "vm_workload_determinism_tiers_test.cpp"
#undef main
#include <array>
#include <cstdlib>
#include <stdexcept>
#include "codec.hpp"
#include "foundation_identity.hpp"
#include "t81/canonfs/canon_driver.hpp"
#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

static void need(bool yes, const char* why) {
  if (!yes) throw std::runtime_error(why);
}
static void quote(std::string_view s) {
  std::cout << '"';
  for (unsigned char c : s) {
    if (c == '"' || c == '\\')
      std::cout << '\\' << char(c);
    else if (c == '\n')
      std::cout << "\\n";
    else if (c == '\r')
      std::cout << "\\r";
    else if (c == '\t')
      std::cout << "\\t";
    else {
      need(c >= 32, "unexpected control byte");
      std::cout << char(c);
    }
  }
  std::cout << '"';
}
template <class Bytes>
static void hex(const Bytes& data) {
  constexpr char digits[] = "0123456789abcdef";
  std::cout << '"';
  for (auto x : data) {
    auto v = static_cast<unsigned char>(x);
    std::cout << digits[v >> 4] << digits[v & 15];
  }
  std::cout << '"';
}
static void arithmetic() {
  std::vector<int64_t> input{INT64_MIN, INT64_MIN + 1, INT64_MAX - 1, INT64_MAX, 0, 1, -1};
  input.reserve(1031);
  uint64_t seed = 0x424153453031ULL;
  for (size_t i = 0; i < 1024; ++i) {
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    auto magnitude = static_cast<int64_t>(seed & 0x7fffffffffffffffULL);
    input.push_back((seed >> 63) ? -magnitude : magnitude);
  }
  std::vector<Digits> encoded(input.size());
  std::vector<int64_t> decoded(input.size());
  encode_all(input, encoded);  // Allocating API; outputs remain live.
  size_t trit_capacity = 0;
  for (const auto& ds : encoded) {
    need(!ds.empty() && ds.size() <= 41 && ds.capacity() <= 64, "codec digit bound");
    need(ds.size() == 1 || ds.back() != t81::Trit::Zero, "noncanonical leading zero");
    for (auto d : ds)
      need(t81::trit_to_int(d) >= -1 && t81::trit_to_int(d) <= 1, "noncanonical digit");
    trit_capacity += ds.capacity();
  }
  decode_all(encoded, decoded);  // Separate API/stage, no encode inside this call.
  need(input == decoded, "decode result mismatch");
  need(input.capacity() <= 1031 && encoded.capacity() <= 1031 && decoded.capacity() <= 1031,
       "outer retention bound");
  std::cout << "\"codec\":{\"encoded\":[";
  for (size_t i = 0; i < input.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << '[' << input[i] << ",[";
    for (size_t j = 0; j < encoded[i].size(); ++j) {
      if (j) std::cout << ',';
      std::cout << t81::trit_to_int(encoded[i][j]);
    }
    std::cout << "]]";
  }
  std::cout << "],\"decoded\":[";
  for (size_t i = 0; i < decoded.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << decoded[i];
  }
  std::cout << "],\"retention\":{\"inputs\":1031,\"encoded_outputs\":1031,\"decoded_outputs\":1031,"
               "\"trit_capacity\":"
            << trit_capacity << ",\"trit_bytes\":" << sizeof(t81::Trit)
            << ",\"vector_object_bytes\":" << sizeof(Digits) << ",\"accounted_container_bytes\":"
            << (trit_capacity * sizeof(t81::Trit) + 1031 * (sizeof(Digits) + 2 * sizeof(int64_t)))
            << "}}";
}
static void canonfs() {
  using namespace t81::canonfs;
  std::cout << ",\"canonfs\":[";
  bool first = true;
  for (size_t size : {4096U, 65536U, 1048576U}) {
    auto driver = make_in_memory_driver();  // Only storage factory in this harness.
    std::vector<std::byte> payload(size, std::byte{0x42});
    std::array<CanonRef, 2> refs;
    std::array<std::vector<std::byte>, 2> reads;
    for (size_t i = 0; i < 2; ++i) {
      auto result = driver->write_object(ObjectType::RawBlock, payload);
      need(result.has_value(), "CanonFS write failed");
      refs[i] = result.value();
    }
    for (size_t i = 0; i < 2; ++i) {
      auto result = driver->read_object_bytes(refs[i]);
      need(result.has_value(), "CanonFS read failed");
      reads[i] = std::move(result.value());
    }
    need(refs[0].hash == refs[1].hash, "repeat identity mismatch");
    for (const auto& read : reads)
      need(read == payload && read.capacity() <= size, "read bytes/bound mismatch");
    need(payload.capacity() <= size, "payload bound");
    if (!first) std::cout << ',';
    first = false;
    std::cout << "{\"bytes\":" << size << ",\"fill\":66,\"hashes\":[";
    hex(refs[0].hash.h.bytes);
    std::cout << ',';
    hex(refs[1].hash.h.bytes);
    std::cout << "],\"read_outputs_hex\":[";
    hex(reads[0]);
    std::cout << ',';
    hex(reads[1]);
    std::cout << "],\"retained_refs\":2,\"retained_reads\":2,\"accounted_payload_bytes\":"
              << 3 * size << '}';
    // Reads and refs outlive native verification and output serialization for this size.
  }
  std::cout << ']';
}
static void vm_checks() {
  const std::vector<std::pair<std::string, t81::tisc::Program>> programs{
      {"workload", workload_program()},
      {"micro", micro_program()},
      {"meso", meso_program()},
      {"mixed", mixed_program()},
      {"policy-heavy", policy_heavy_program()},
      {"tensor-access", tensor_access_program()}};
  const std::array<int64_t, 6> ticks{406, 5, 250, 11, 7, 8};  // PR #690; PROVISIONAL.
  std::cout << ",\"vm\":[";
  for (size_t n = 0; n < programs.size(); ++n) {
    const auto& [name, p] = programs[n];
    auto machine = t81::vm::make_interpreter_vm();  // Fresh object for every builder.
    machine->load_program(p);
    need(machine->state().contexts.size() == 1 && machine->state().contexts[0].registers[75] == 0,
         "VM not fresh/single context");
    auto result = machine->run_to_halt(5000);
    const auto& s = machine->state();
    const auto& c = s.contexts.at(0);
    need(result.has_value() && s.halted && c.pc == p.insns.size(), "VM incomplete/faulted");
    need(!s.trace.empty() && s.trace.back().pc == p.insns.size() - 1 &&
             s.trace.back().opcode == t81::tisc::Opcode::Halt && !s.trace.back().trap,
         "VM explicit Halt missing");
    for (const auto& event : s.trace) need(!event.trap, "VM trap recorded");
    for (const auto& event : s.axion_log)
      need(event.verdict.kind != t81::axion::VerdictKind::Deny, "VM policy denial");
    need(c.registers[75] == ticks[n], "VM provisional ledger mismatch");
    need(s.memory.size() == 1536 + p.insns.size() && s.trace.size() <= 512 &&
             s.axion_log.size() <= 2048,
         "VM fixed-profile retention bound");
    if (n) std::cout << ',';
    std::cout << "{\"tier\":";
    quote(name);
    std::cout << ",\"status\":\"PROVISIONAL\",\"initial_r75\":0,\"contexts\":1,\"halted\":true,"
                 "\"terminal_pc\":"
              << c.pc << ",\"final_r75\":" << c.registers[75]
              << ",\"memory_words\":" << s.memory.size() << ",\"trace_entries\":" << s.trace.size()
              << ",\"axion_events\":" << s.axion_log.size() << ",\"inputs\":[";
    for (size_t i = 0; i < p.insns.size(); ++i) {
      if (i) std::cout << ',';
      const auto& x = p.insns[i];
      std::cout << '[' << int(x.opcode) << ',' << x.a << ',' << x.b << ',' << x.c << ','
                << int(x.literal_kind) << ']';
    }
    std::cout << "],\"policy\":";
    quote(p.axion_policy_text);
    std::cout << ",\"registers\":[";
    for (size_t i = 0; i < 48; ++i) {
      if (i) std::cout << ',';
      std::cout << c.registers[i];
    }
    std::cout << "],\"tags\":[";
    for (size_t i = 0; i < 48; ++i) {
      if (i) std::cout << ',';
      std::cout << int(c.register_tags[i]);
    }
    std::cout << "],\"floats\":[";
    for (size_t i = 0; i < s.floats.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << s.floats[i];
    }
    std::cout << "],\"printed\":[";
    for (size_t i = 0; i < s.printed_output.size(); ++i) {
      if (i) std::cout << ',';
      quote(s.printed_output[i]);
    }
    std::cout << "]}";
  }
  std::cout << ']';
}
int main(int argc, char** argv) {
  if (argc != 2 ||
      (std::string(argv[1]) != "--validate-only" && std::string(argv[1]) != "--identity-only")) {
    std::cerr << "measurement unavailable; --validate-only required\n";
    return 2;
  }
  if (std::string(argv[1]) == "--identity-only") {
    std::cout << "{\"native_identity\":\"" << FOUNDATION_NATIVE_IDENTITY << "\"}\n";
    return 0;
  }
#if !defined(__linux__) && !defined(__APPLE__)
  std::cerr << "unsupported worker platform\n";
  return 2;
#else
  if (std::getenv("T81_CANONFS_ROOT") || !std::getenv("T81_CANONFS_READ_VERIFY") ||
      std::string(std::getenv("T81_CANONFS_READ_VERIFY")) != "1") {
    std::cerr << "require no CanonFS root and explicit read verification\n";
    return 2;
  }
  try {
    std::cout << "{\"native_identity\":\"" << FOUNDATION_NATIVE_IDENTITY
              << "\",\"worker_pid\":" << getpid() << ",\"parent_pid\":" << getppid() << ',';
    arithmetic();
    canonfs();
    vm_checks();
    std::cout << "}\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
#endif
}
