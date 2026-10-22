// Opt-in research adapter; production execution, policy, storage and hashing.
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif
#include "t81/canonfs/axion_hook.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/vm/vm.hpp"

using Clock = std::chrono::steady_clock;
using namespace t81;
std::string json_quote(const std::string& s) {
  std::ostringstream o;
  o << '"';
  for (unsigned char c : s) {
    if (c == '"' || c == '\\')
      o << '\\' << c;
    else if (c < 32)
      o << "\\u" << std::hex << std::setfill('0') << std::setw(4) << unsigned(c);
    else
      o << c;
  }
  return o.str() + '"';
}
std::string digest(const std::string& s) {
  std::ostringstream o;
  for (auto b : hash::hash_string(s).bytes)
    o << std::hex << std::setfill('0') << std::setw(2) << unsigned(b);
  return o.str();
}
long long elapsed(Clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}
tisc::Program workload(int x, int divisor, const std::string& mode) {
  using O = tisc::Opcode;
  tisc::Program p;
  p.axion_policy_text = mode == "baseline" ? "" : "(policy (tier 1) (max-instructions 64))\n";
  p.insns = {{O::LoadImm, 40, x, 0},   {O::LoadImm, 41, 2, 0},  {O::Add, 42, 40, 41},
             {O::LoadImm, 43, 3, 0},   {O::Mul, 42, 42, 43},    {O::LoadImm, 46, divisor, 0},
             {O::Div, 44, 42, 46},     {O::Mod, 45, 42, 43},    {O::JumpIfZero, 10, 45, 0},
             {O::LoadImm, 44, 999, 0}, {O::Store, 1024, 44, 0}, {O::Halt, 0, 0, 0}};
  // deny-vm budget is nine: nine dispatched instructions, then deny Store.
  if (mode == "deny-vm") p.axion_policy_text = "(policy (tier 1) (max-instructions 9))\n";
  return p;
}
int main(int argc, char** argv) {
  try {
    std::locale::global(std::locale::classic());
    if (argc != 5)
      throw std::runtime_error("usage: runner INPUT(0..100) DIVISOR(0..3) "
                               "allow|deny-vm|deny-store|baseline FRESH_STORE");
    const std::string xs = argv[1], ds = argv[2], mode = argv[3];
    const int x = std::stoi(xs), divisor = std::stoi(ds);
    if (x < 0 || x > 100 || divisor < 0 || divisor > 3 || xs != std::to_string(x) ||
        ds != std::to_string(divisor))
      throw std::runtime_error("noncanonical or out-of-range input");
    if (mode != "allow" && mode != "deny-vm" && mode != "deny-store" && mode != "baseline")
      throw std::runtime_error("unknown profile");
    const std::filesystem::path root = argv[4];
    if (std::filesystem::exists(root)) throw std::runtime_error("store must be fresh");
    // Warm the production interpreter and hash code five times in this process.
    // Store creation remains cold and is included in the storage measurement.
    for (int n = 0; n < 5; ++n) {
      auto warm = vm::make_interpreter_vm();
      warm->load_program(workload(x, divisor, mode));
      (void)warm->run_to_halt(64);
      (void)digest("warm-up");
    }
    const auto start = Clock::now();
    auto p = workload(x, divisor, mode);
    std::string program;
    for (const auto& i : p.insns)
      program += std::string(tisc::opcode_name(i.opcode)) + " " + std::to_string(i.a) + " " +
                 std::to_string(i.b) + " " + std::to_string(i.c) + " 0\n";
    auto machine = vm::make_interpreter_vm();
    machine->load_program(p);
    const auto setup_ns = elapsed(start);
    const auto core_start = Clock::now();
    auto result = machine->run_to_halt(64);
    const auto execution_ns = elapsed(core_start);
    const auto& state = machine->state();
    const auto serialization_start = Clock::now();
    std::string trace;
    for (const auto& e : state.trace)
      trace += std::to_string(e.pc) + " " + std::string(tisc::opcode_name(e.opcode)) + " " +
               vm::to_string(e.trap.value_or(vm::Trap::None)) + "\n";
    std::string decisions;
    for (const auto& e : state.axion_log)
      decisions += std::string(tisc::opcode_name(e.opcode)) + " " +
                   std::to_string(static_cast<int>(e.verdict.kind)) + " " + e.verdict.reason + "\n";
    const std::string output = std::to_string(state.memory.at(1024)) + "\n";
    const auto trace_hash = digest(trace);
    const auto serialization_hash_ns = elapsed(serialization_start);
    const auto storage_start = Clock::now();
    const std::string storage_policy = mode == "baseline" ? ""
                                       : mode == "deny-store"
                                           ? "(policy (tier 1) (max-meta-writes 0))\n"
                                           : "(policy (tier 1))\n";
    std::string reference;
    if (result) {
      auto driver = canonfs::make_persistent_driver(root);
      if (mode != "baseline")
        driver->set_axion_hook(canonfs::make_axion_policy_hook(storage_policy));
      auto stored = driver->write_object(canonfs::ObjectType::RawBlock,
                                         std::as_bytes(std::span(output.data(), output.size())));
      if (stored) {
        reference = stored->hash.h.to_string();
        // Read under an explicit read-allow policy, independent of write denial.
        driver->set_axion_hook(canonfs::make_axion_policy_hook("(policy (tier 1))"));
        auto restored = driver->read_object_bytes(*stored);
        if (!restored ||
            *restored != std::vector<std::byte>(
                             std::as_bytes(std::span(output.data(), output.size())).begin(),
                             std::as_bytes(std::span(output.data(), output.size())).end()))
          throw std::runtime_error("CanonFS readback mismatch");
      } else if (stored.error() != canonfs::Error::CapabilityError)
        throw std::runtime_error("unexpected storage failure");
    }
    const auto storage_ns = elapsed(storage_start);
    const auto envelope_start = Clock::now();
    // Canonical float conversion probes, distinct from VM FloatHandle semantics.
    using F = v1::T81Float<27, 9>;
    const double values[] = {0.0,
                             -0.0,
                             std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::denorm_min(),
                             std::numeric_limits<double>::min(),
                             std::numeric_limits<double>::max(),
                             -std::numeric_limits<double>::max()};
    std::string numerics;
    const T81BigInt max(std::numeric_limits<std::int64_t>::max());
    const T81BigInt one(1), three(3);
    const auto wide_sum = max + one;
    const std::string integer_boundaries = wide_sum.to_string() + "\n" + (max * three).to_string() +
                                           "\n" + (wide_sum / one).to_string() + "\n";
    for (double v : values) numerics += F::from_double(v).to_canonical_string() + "\n";
    std::ostringstream record;
    record << "{\"version\":2,\"input\":" << json_quote(xs + " " + ds + "\n")
           << ",\"profile\":" << json_quote(mode) << ",\"program\":" << json_quote(program)
           << ",\"program_hash\":" << json_quote(digest(program))
           << ",\"input_hash\":" << json_quote(digest(xs + " " + ds + "\n"))
           << ",\"vm_policy\":" << json_quote(p.axion_policy_text)
           << ",\"vm_policy_hash\":" << json_quote(digest(p.axion_policy_text))
           << ",\"storage_policy\":" << json_quote(storage_policy)
           << ",\"storage_policy_hash\":" << json_quote(digest(storage_policy))
           << ",\"trap\":" << json_quote(result ? "None" : vm::to_string(result.error()))
           << ",\"trace\":" << json_quote(trace) << ",\"trace_hash\":" << json_quote(trace_hash)
           << ",\"decisions\":" << json_quote(decisions) << ",\"output\":" << json_quote(output)
           << ",\"output_hash\":" << json_quote(digest(output))
           << ",\"reference\":" << json_quote(reference)
           << ",\"integer_boundaries\":" << json_quote(integer_boundaries) << "}";
    const auto envelope_ns = elapsed(envelope_start);
    const auto total_ns = elapsed(start);
    long long peak_rss = -1;
#if defined(__APPLE__) || defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
      peak_rss = usage.ru_maxrss;
#if defined(__linux__)
      peak_rss *= 1024;
#endif
    }
#endif
    // Unsupported host-dependent conversions are diagnostics, never canonical
    // evidence. Preserve them explicitly so a valid integer replay cannot be
    // misread as a passing float-conversion conformance test.
    std::cout << "{\"record\":" << record.str()
              << ",\"diagnostics\":{\"status\":\"UNSUPPORTED_NONCANONICAL\",\"float_conversions\":"
              << json_quote(numerics) << "},\"measurements\":{\"setup_ns\":" << setup_ns
              << ",\"envelope_ns\":" << envelope_ns << ",\"process_peak_rss_bytes\":" << peak_rss
              << ",\"execution_ns\":" << execution_ns
              << ",\"serialization_hash_ns\":" << serialization_hash_ns
              << ",\"storage_ns\":" << storage_ns << ",\"total_ns\":" << total_ns
              << ",\"trace_capacity_bytes\":" << state.trace.capacity() * sizeof(vm::TraceEntry)
              << ",\"audit_capacity_bytes\":" << state.axion_log.capacity() * sizeof(vm::AxionEvent)
              << "},\"compiler\":"
              << json_quote(
#ifdef __clang__
                     "Clang " __clang_version__
#elif defined(__GNUC__)
                     "GCC " __VERSION__
#else
                     "unknown"
#endif
                     )
              << "}\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
