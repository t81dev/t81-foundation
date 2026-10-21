// Experiment-local, fixture-bounded preflight. No public object API.
#include <algorithm>
#include <array>
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
namespace {
std::string read(const fs::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("read failed");
  return {std::istreambuf_iterator<char>(stream), {}};
}
void write(const fs::path& path, const std::string& data) {
  std::ofstream stream(path, std::ios::binary);
  stream.write(data.data(), static_cast<std::streamsize>(data.size()));
  if (!stream) throw std::runtime_error("write failed");
}
auto bytes(const std::string& data) {
  return std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()), data.size());
}
std::string digest(const std::string& data) {
  std::ostringstream stream;
  for (auto b : t81::hash::hash_string(data).bytes)
    stream << std::hex << std::setfill('0') << std::setw(2) << unsigned(b);
  return stream.str();
}
void require(bool ok) {
  if (!ok) throw std::runtime_error("experiment invariant failed");
}
t81::tisc::Program program() {
  t81::tisc::Program p;
  p.insns = {{t81::tisc::Opcode::LoadImm, 40, 6, 0},
             {t81::tisc::Opcode::LoadImm, 41, 7, 0},
             {t81::tisc::Opcode::Mul, 42, 40, 41},
             {t81::tisc::Opcode::Halt, 0, 0, 0}};
  return p;
}
std::string tensor() {
  std::string header = R"({"weight":{"dtype":"I8","shape":[2,2],"data_offsets":[0,4]}})";
  header.append((8 - header.size() % 8) % 8, ' ');
  std::string data;
  const auto size = static_cast<uint64_t>(header.size());
  for (unsigned i = 0; i < 8; ++i) data.push_back(static_cast<char>((size >> (8 * i)) & 255));
  return data + header + std::string("\xff\x00\x01\xff", 4);
}
std::string run_tisc(const std::string& data, const fs::path&) {
  auto parsed = t81::tisc::base81_view::parse(data);
  require(static_cast<bool>(parsed));
  auto vm = t81::vm::make_interpreter_vm(t81::axion::make_policy_engine(t81::axion::Policy{}));
  vm->load_program(parsed.value());
  require(vm->run_to_halt(100).has_value());
  return std::to_string(vm->state().contexts[0].registers[42]) + "\n";
}
std::string inspect_tensor(const std::string& data, const fs::path& work) {
  const auto snapshot = work / "snapshot";
  write(snapshot, data);
  const auto model = t81::weights::load_safetensors(snapshot);
  require(model.tensors.size() == 1 && model.tensors[0].name == "weight" &&
          model.tensors[0].shape == std::vector<uint64_t>({2, 2}));
  return std::to_string(model.tensors.size()) + ":" + std::to_string(model.total_parameters) + "\n";
}
struct Adapter {
  std::string representation, provider, operation, fixture;
  std::string (*invoke)(const std::string&, const fs::path&);
};
const std::array<Adapter, 2> adapters{
    {{"base81-symbolic-tisc", "t81vm", "run", t81::tisc::base81_view::render(program()), run_tisc},
     {"safetensors-i8", "native-weights", "inspect", tensor(), inspect_tensor}}};
struct Descriptor {
  std::array<std::string, 6> fields;
};
Descriptor decode(const std::string& raw) {
  if (raw.size() > 4096 || raw.empty() || raw.back() != '\n') throw std::runtime_error("malformed");
  std::istringstream stream(raw);
  Descriptor d;
  for (auto& field : d.fields) {
    if (!std::getline(stream, field) || field.empty() ||
        !std::all_of(field.begin(), field.end(),
                     [](unsigned char c) { return c >= 33 && c <= 126; }))
      throw std::runtime_error("malformed");
  }
  std::string extra;
  if (std::getline(stream, extra)) throw std::runtime_error("malformed");
  if (d.fields[1].size() != 64 || !std::all_of(d.fields[1].begin(), d.fields[1].end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
      }))
    throw std::runtime_error("malformed");
  return d;
}
// Returns only a validated adapter; caller orchestration has no representation branch.
const Adapter& resolve(const Descriptor& d, const std::string& input, const std::string& expected) {
  if (d.fields[0] != "t81.experiment-descriptor.v1") throw std::runtime_error("version_denied");
  if (d.fields[1] != expected || digest(input) != expected)
    throw std::runtime_error("payload_denied");
  if (d.fields[5] != "object-interface-fixtures-v1") throw std::runtime_error("constraint_denied");
  const auto found = std::find_if(adapters.begin(), adapters.end(), [&](const Adapter& a) {
    return a.representation == d.fields[2] && a.provider == d.fields[3];
  });
  if (found == adapters.end()) throw std::runtime_error("provider_denied");
  // Exact fixture validation is deliberate: no second general SafeTensors parser.
  // A matching content hash by itself does not validate the representation claim.
  if (input != found->fixture) throw std::runtime_error("representation_denied");
  if (d.fields[4] != found->operation) throw std::runtime_error("capability_denied");
  return *found;
}
}  // namespace
int main(int argc, char** argv) {
  try {
    if (argc != 9)
      throw std::runtime_error("usage: runner descriptor descriptor-ref source payload-ref "
                               "operation allow|deny scenario work");
    const std::string raw = read(argv[1]), input = read(argv[3]);
    const std::string descriptor_ref = digest(raw), payload_ref = digest(input);
    const fs::path work = argv[8];
    const std::string scenario = argv[7];
    const std::string allow = "(policy (tier 1))";
    const std::string deny = "(policy (tier 1) (require-axion-event (reason \"absent\")))";
    std::string status, supported;
    unsigned invocations = 0;
    // Allowed intake and verified read are independent of computational permission.
    auto driver = make_persistent_driver(work / "store");
    driver->set_axion_hook(make_axion_policy_hook(allow));
    auto ref = driver->write_object(ObjectType::RawBlock, bytes(input));
    require(ref.has_value() && digest(input) == payload_ref);
    auto restored = driver->read_object_bytes(*ref);
    require(restored.has_value() &&
            *restored == std::vector<std::byte>(bytes(input).begin(), bytes(input).end()));
    write(work / "restored", input);
    try {
      if (descriptor_ref != argv[2]) throw std::runtime_error("descriptor_denied");
      const auto d = decode(raw);
      const auto& adapter = resolve(d, input, argv[4]);
      supported = adapter.operation;
      if (supported != argv[5]) throw std::runtime_error("unsupported");
      if (scenario == "storage_denial") {
        CapabilityGrant grant{*ref, {"experiment", ""}, CANON_PERM_WRITE, {}, 0, {}};
        require(driver->publish_capability(grant).has_value());
        auto denied_read = driver->read_object_bytes(*ref);
        require(!denied_read && denied_read.error() == Error::CapabilityError);
        throw std::runtime_error("storage_denied");
      }
      auto policy = t81::axion::parse_policy(std::string(argv[6]) == "allow" ? allow : deny);
      require(policy.has_value());
      auto engine = t81::axion::make_policy_engine(*policy);
      t81::axion::SyscallContext ctx;
      ctx.caller = "descriptor-experiment";
      ctx.syscall = supported;
      ctx.payload = payload_ref;
      ctx.final_authorization = true;
      if (engine->evaluate(ctx).kind != t81::axion::VerdictKind::Allow)
        throw std::runtime_error("operation_denied");
      if (scenario == "source_mutation") write(argv[3], input + "changed");
      // Count attempted invocations, before any provider parsing/loading/execution.
      ++invocations;
      write(work / "output", adapter.invoke(input, work));
      status = "ok";
    } catch (const std::runtime_error& error) {
      status = error.what();
      const std::array<std::string, 12> errors{
          "malformed",       "version_denied",        "payload_denied",    "constraint_denied",
          "provider_denied", "representation_denied", "capability_denied", "descriptor_denied",
          "unsupported",     "storage_denied",        "operation_denied",  "ok"};
      require(std::find(errors.begin(), errors.end(), status) != errors.end());
    }
    std::cout << "{\"status\":\"" << status << "\",\"payload_ref\":\"" << payload_ref
              << "\",\"descriptor_ref\":\"" << descriptor_ref << "\",\"supported\":\"" << supported
              << "\",\"provider_invocations\":" << invocations << ",\"storage_succeeded\":true}\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
