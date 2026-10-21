// Experiment harness only: no new public runtime or object contract.
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/axion_hook.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/vm.hpp"

namespace fs = std::filesystem;
std::string read(const fs::path& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot read input");
  return {std::istreambuf_iterator<char>(f), {}};
}
std::string digest(const std::string& s) {
  const auto h = t81::hash::hash_string(s);
  std::ostringstream out;
  for (auto b : h.bytes) out << std::hex << std::setfill('0') << std::setw(2) << unsigned(b);
  return out.str();
}
std::span<const std::byte> bytes(const std::string& s) {
  return {reinterpret_cast<const std::byte*>(s.data()), s.size()};
}
class CountedPolicy final : public t81::axion::Engine {
public:
  explicit CountedPolicy(std::size_t& calls) : calls_(calls), engine_(t81::axion::Policy{}) {}
  t81::axion::Verdict evaluate(const t81::axion::SyscallContext& ctx) override {
    ++calls_;
    return engine_.evaluate(ctx);
  }

private:
  std::size_t& calls_;
  t81::axion::PolicyEngine engine_;
};
int main(int argc, char** argv) {
  try {
    if (argc != 7)
      throw std::runtime_error(
          "usage: runner B|C|D input expected-digest policy output-store revoke-policy-or-dash");
    const std::string arm = argv[1];
    if (arm != "B" && arm != "C" && arm != "D") throw std::runtime_error("unknown arm");
    const auto input = read(argv[2]);
    const auto input_hash = digest(input);
    const std::string expected = argv[3];
    std::string policy = read(argv[4]);
    const fs::path store = argv[5];
    if (fs::exists(store)) throw std::runtime_error("output store must be fresh");
    std::string status = "identity_denied", output;
    std::size_t vm_policy_calls = 0, policy_calls = 0;
    bool committed = false;
    if (input_hash == expected) {
      auto intake = t81::canonfs::make_in_memory_driver();
      auto imported = intake->write_object(t81::canonfs::ObjectType::RawBlock, bytes(input));
      if (!imported) throw std::runtime_error("import failed");
      auto restored = intake->read_object_bytes(*imported);
      if (!restored ||
          *restored != std::vector<std::byte>(bytes(input).begin(), bytes(input).end()))
        throw std::runtime_error("import round trip failed");
      int units = 0, price = 0;
      std::istringstream fields(input);
      std::string extra;
      if (!(fields >> units >> price) || (fields >> extra) || units < 1 || units > 1000 ||
          price < 1 || price > 1000)
        throw std::runtime_error("expected two integers in [1,1000]");
      t81::tisc::Program program;
      program.insns = {{t81::tisc::Opcode::LoadImm, 40, units, 0},
                       {t81::tisc::Opcode::LoadImm, 41, price, 0},
                       {t81::tisc::Opcode::Mul, 42, 40, 41},
                       {t81::tisc::Opcode::Halt, 0, 0, 0}};
      auto vm = t81::vm::make_interpreter_vm(std::make_unique<CountedPolicy>(vm_policy_calls));
      vm->load_program(program);
      if (!vm->run_to_halt()) throw std::runtime_error("VM execution failed");
      output = std::to_string(vm->state().contexts[0].registers[42]) + "\n";
      // Deterministic revocation barrier: replace active policy after computation.
      if (std::string(argv[6]) != "-") policy = read(argv[6]);
      std::optional<t81::axion::Policy> parsed;
      // D delegates parsing and all authorization to the production hook.
      if (arm != "D") {
        auto candidate = t81::axion::parse_policy(policy);
        if (candidate) parsed = std::move(*candidate);
      }
      status = "policy_denied";
      // C validates and requests final policy evaluation at the commit boundary.
      // The existing text hook checks these requirements only for Opcode::Halt.
      if (arm == "B" || arm == "D" || parsed.has_value()) {
        t81::canonfs::reset_axion_trace();
        auto hook = t81::canonfs::make_axion_policy_hook(policy);
        std::unique_ptr<t81::axion::Engine> boundary_engine;
        if (arm == "C") {
          boundary_engine = t81::axion::make_policy_engine(std::move(*parsed));
        }
        auto driver = t81::canonfs::make_persistent_driver(store);
        driver->set_axion_hook([&](auto kind, const auto& ref) {
          ++policy_calls;
          if (boundary_engine) {
            t81::axion::SyscallContext context;
            context.caller = "accountable-experiment";
            context.syscall = "canonfs-Write";
            context.current_tier = 1;
            // Existing PolicyEngine uses Halt as its final-evaluation marker.
            // This is an adapter convention, not a VM instruction executed here.
            context.next_opcode = t81::tisc::Opcode::Halt;
            auto verdict = boundary_engine->evaluate(context);
            return t81::canonfs::AxionVerdict{verdict.kind == t81::axion::VerdictKind::Allow,
                                              verdict.reason};
          }
          return hook(kind, ref);
        });
        auto result = driver->write_object(t81::canonfs::ObjectType::RawBlock, bytes(output));
        committed = result.has_value();
        if (!committed && result.error() != t81::canonfs::Error::CapabilityError)
          throw std::runtime_error("unexpected storage failure");
        status = committed ? "committed" : "policy_denied";
      }
    }
    std::cout << "{\"status\":\"" << status << "\",\"input_digest\":\"" << input_hash
              << "\",\"policy_digest\":\"" << digest(policy) << "\",\"output_digest\":\""
              << (committed ? digest(output) : "")
              << "\",\"amount\":" << (output.empty() ? 0 : std::stoi(output))
              << ",\"vm_policy_calls\":" << vm_policy_calls << ",\"policy_calls\":" << policy_calls
              << "}\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
