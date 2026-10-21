// Bounded experiment adapter, deliberately not a public computational-object API.
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/axion_hook.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/isa/base81_view.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

namespace fs = std::filesystem;
using namespace t81::canonfs;
std::string read(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) throw std::runtime_error("read failed");
  return {std::istreambuf_iterator<char>(f), {}};
}
void write(const fs::path& p, const std::string& s) {
  std::ofstream f(p, std::ios::binary);
  f.write(s.data(), static_cast<std::streamsize>(s.size()));
  if (!f) throw std::runtime_error("write failed");
}
auto bytes(const std::string& s) {
  return std::span<const std::byte>(reinterpret_cast<const std::byte*>(s.data()), s.size());
}
std::string hex(const t81::hash::CanonHash81& h) {
  std::ostringstream s;
  for (auto b : h.bytes) s << std::hex << std::setfill('0') << std::setw(2) << unsigned(b);
  return s.str();
}
void check(bool b) {
  if (!b) throw std::runtime_error("experiment invariant failed");
}

int main(int argc, char** argv) {
  try {
    if (argc == 3 && std::string(argv[1]) == "construct") {
      t81::tisc::Program p;
      p.insns = {{t81::tisc::Opcode::LoadImm, 40, 6, 0},
                 {t81::tisc::Opcode::LoadImm, 41, 7, 0},
                 {t81::tisc::Opcode::Mul, 42, 40, 41},
                 {t81::tisc::Opcode::Halt, 0, 0, 0}};
      write(argv[2], t81::tisc::base81_view::render(p));
      return 0;
    }
    if (argc != 6)
      throw std::runtime_error("usage: runner tisc|safetensors case source expected-hash workdir");
    const std::string kind = argv[1], scenario = argv[2];
    check(kind == "tisc" || kind == "safetensors");
    const fs::path source = argv[3], work = argv[5];
    const auto input = read(source);  // one immutable owned snapshot
    const auto identity = hex(t81::hash::hash_string(input));
    const std::string allow = "(policy (tier 1))";
    const std::string deny = "(policy (tier 1) (require-axion-event (reason \"absent\")))";
    const auto policy = scenario == "policy_denial" ? deny : allow;
    std::string status = "identity_denied", output, output_ref, provenance_ref;
    std::string inspection = "null";
    std::size_t completed_operations = 0;
    if (identity == argv[4]) {
      auto driver = make_persistent_driver(work / "store");
      driver->set_axion_hook(make_axion_policy_hook(allow));
      auto ref = driver->write_object(ObjectType::RawBlock, bytes(input));
      check(ref.has_value() && hex(ref->hash.h) == identity);
      auto restored = driver->read_object_bytes(*ref);
      check(restored.has_value() &&
            *restored == std::vector<std::byte>(bytes(input).begin(), bytes(input).end()));
      write(work / "restored", input);
      // ObjectType is a storage tag, not a checked representation declaration.
      auto other = driver->write_object(ObjectType::CanonTensor, bytes(input));
      check(other.has_value() && other->hash == ref->hash);
      if (scenario == "capability_denial") {
        CapabilityGrant grant{*ref, {"experiment", ""}, CANON_PERM_WRITE, {}, 0, {}};
        check(driver->publish_capability(grant).has_value());
        auto denied = driver->read_object_bytes(*ref);
        check(!denied && denied.error() == Error::CapabilityError);
        status = "capability_denied";
      } else if (scenario == "unsupported") {
        // No common supports(operation) API: this is an adapter decision only.
        status = "adapter_unsupported";
      } else if (scenario == "policy_denial") {
        driver->set_axion_hook(make_axion_policy_hook(policy));
        auto denied = driver->read_object_bytes(*ref);
        check(!denied && denied.error() == Error::CapabilityError);
        status = "policy_denied";
      } else {
        if (scenario == "changed_after_verify") write(source, input + "changed");
        // Model loader requires a path. Materialize verified bytes in a private
        // experiment directory; the caller's mutable source is never reopened.
        const auto snapshot = work / "snapshot";
        write(snapshot, input);
        const auto representation =
            scenario == "wrong_representation" ? (kind == "tisc" ? "safetensors" : "tisc") : kind;
        try {
          if (representation == "tisc") {
            auto parsed = t81::tisc::base81_view::parse(input);
            if (!parsed || parsed.value().insns.empty())
              throw std::runtime_error("invalid program");
            inspection = "{\"representation\":\"base81-symbolic-tisc\",\"instructions\":" +
                         std::to_string(parsed.value().insns.size()) + "}";
            auto vm =
                t81::vm::make_interpreter_vm(t81::axion::make_policy_engine(t81::axion::Policy{}));
            vm->load_program(parsed.value());
            check(vm->run_to_halt(100).has_value());
            ++completed_operations;
            output = std::to_string(vm->state().contexts[0].registers[42]) + "\n";
          } else {
            auto model = t81::weights::load_safetensors(snapshot);
            check(model.tensors.size() == 1 && model.tensors[0].name == "weight" &&
                  model.tensors[0].shape == std::vector<uint64_t>({2, 2}));
            inspection =
                "{\"representation\":\"safetensors-i8\",\"tensor\":\"weight\",\"shape\":[2,2]}";
            ++completed_operations;
            // Intake/inspection, explicitly not inference or VM execution.
            output = std::to_string(model.tensors.size()) + ":" +
                     std::to_string(model.total_parameters) + "\n";
          }
          status = "ok";
        } catch (const std::exception&) {
          if (scenario != "wrong_representation") throw;
          status = "representation_rejected";
        }
        if (status == "ok") {
          auto result = driver->write_object(ObjectType::RawBlock, bytes(output));
          check(result.has_value());
          output_ref = hex(result->hash.h);
          write(work / "output", output);
          std::string provenance = "object-interface-experiment-v1\n" + identity + "\n" + kind +
                                   "\n" + (kind == "tisc" ? "run" : "inspect") + "\n" +
                                   hex(t81::hash::hash_string(policy)) + "\n" + output_ref + "\n";
          if (scenario == "invalid_provenance")
            provenance.replace(provenance.size() - 65, 64, 64, '0');
          if (scenario == "incomplete_provenance") provenance = "object-interface-experiment-v1\n";
          auto record = driver->write_object(ObjectType::RawBlock, bytes(provenance));
          check(record.has_value());
          provenance_ref = hex(record->hash.h);
          write(work / "provenance", provenance);
          auto reread = driver->read_object_bytes(*record);
          check(reread.has_value() && *reread == std::vector<std::byte>(bytes(provenance).begin(),
                                                                        bytes(provenance).end()));
        }
      }
    }
    std::cout << "{\"status\":\"" << status << "\",\"input_ref\":\"" << identity
              << "\",\"output_ref\":\"" << output_ref << "\",\"provenance_ref\":\""
              << provenance_ref << "\",\"completed_operations\":" << completed_operations
              << ",\"inspection\":" << inspection << "}\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
