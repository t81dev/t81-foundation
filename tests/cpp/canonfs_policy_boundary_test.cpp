#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

#include "dev/block_device.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/axion_hook.hpp"
#include "test_runtime_check.hpp"

namespace fs = std::filesystem;
using namespace t81::canonfs;
using namespace t81::axion;

namespace {
const std::string allow = "(policy (tier 1))";
const std::string missing_event =
    "(policy (tier 1) (require-axion-event (reason \"commit-authorized\")))";
std::map<fs::path, std::string> snapshot(const fs::path& root) {
  std::map<fs::path, std::string> files;
  for (const auto& entry : fs::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) continue;
    std::ifstream stream(entry.path(), std::ios::binary);
    files.emplace(fs::relative(entry.path(), root),
                  std::string(std::istreambuf_iterator<char>(stream), {}));
  }
  return files;
}
struct Cleanup {
  fs::path root;
  ~Cleanup() {
    std::error_code ec;
    fs::remove_all(root, ec);
  }
};
// Instrument the actual block driver's device-write boundary, including its
// noexcept callback path. No write should reach the device on evaluation error.
class CountedDevice final : public t81::ternaryos::dev::IBlockDevice {
public:
  std::size_t writes{0};
  t81::ternaryos::dev::BlockDeviceInfo info() const noexcept override { return {}; }
  uint64_t block_count() const noexcept override { return 16; }
  bool read_block(uint64_t, t81::ternaryos::dev::BlockData& out) const override {
    out.fill(0);
    return true;
  }
  bool write_block(uint64_t, const t81::ternaryos::dev::BlockData&) override {
    ++writes;
    return true;
  }
  bool flush() override { return true; }
};
}  // namespace

int main() {
  const auto root = fs::temp_directory_path() /
                    ("canonfs-policy-boundary-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Cleanup cleanup{root};
  auto driver = make_persistent_driver(root);
  const std::string payload = "authorized result";
  const auto bytes = std::as_bytes(std::span(payload.data(), payload.size()));
  const auto empty = snapshot(root);
  auto denied = [&] {
    const auto before = snapshot(root);
    auto result = driver->write_object(ObjectType::RawBlock, bytes);
    T81_TEST_CHECK(!result && result.error() == Error::CapabilityError);
    T81_TEST_CHECK(snapshot(root) == before);
  };

  // Explicitly unconfigured low-level driver retains the documented bootstrap
  // behavior. This is not a configured (but missing/invalid) policy.
  auto bootstrap = driver->write_object(ObjectType::RawBlock, bytes);
  T81_TEST_CHECK(bootstrap);
  T81_TEST_CHECK(snapshot(root).size() == empty.size() + 1);
  driver->set_axion_hook([](auto, const auto&) { return AxionVerdict{false, "explicit deny"}; });
  denied();  // Includes the existing-object fast path: authorization still runs.

  driver->set_axion_hook(make_axion_policy_hook(allow));
  T81_TEST_CHECK(driver->write_object(ObjectType::RawBlock, bytes));
  // An observable, different object ensures a denial cannot hide behind EEXIST.
  const std::string second = "must never persist";
  const auto second_bytes = std::as_bytes(std::span(second.data(), second.size()));
  auto deny_new = [&] {
    const auto before = snapshot(root);
    auto result = driver->write_object(ObjectType::RawBlock, second_bytes);
    T81_TEST_CHECK(!result && result.error() == Error::CapabilityError);
    T81_TEST_CHECK(snapshot(root) == before);
  };
  driver->set_axion_hook([](auto, const auto&) { return AxionVerdict{false, "explicit deny"}; });
  deny_new();
  driver->set_axion_hook([](auto, const auto&) { return AxionVerdict{}; });
  deny_new();
  driver->set_axion_hook(make_axion_policy_hook("(policy (max-meta-writes 0))"));
  deny_new();
  for (const auto& text :
       {std::string{}, std::string("invalid-policy"), std::string("(policy (tier 1)) (policy)"),
        std::string("(policy (tier 1)) @"),
        std::string("(policy (max-instructions 99999999999999999999999))"),
        std::string("(policy (max-instructions -1))"), std::string("(policy (tier 4294967297))"),
        missing_event}) {
    driver->set_axion_hook(make_axion_policy_hook(text));
    deny_new();
  }

  // Synchronous revocation: replace the policy after proposal, before commit.
  driver->set_axion_hook(make_axion_policy_hook(allow));
  const auto proposed_result = second_bytes;
  driver->set_axion_hook(make_axion_policy_hook(missing_event));
  const auto before_revoke = snapshot(root);
  auto revoked = driver->write_object(ObjectType::RawBlock, proposed_result);
  T81_TEST_CHECK(!revoked && revoked.error() == Error::CapabilityError);
  T81_TEST_CHECK(snapshot(root) == before_revoke);

  driver->set_axion_hook([](auto, const auto&) -> AxionVerdict {
    throw std::runtime_error("injected policy evaluation failure");
  });
  deny_new();

  // Prior operations, other callbacks, and global audit history are not proof
  // that the current operation has supplied its required authorization event.
  auto read_policy =
      make_axion_policy_hook("(policy (require-axion-event (reason \"action=Read\")))");
  T81_TEST_CHECK(read_policy(OpKind::Read, *bootstrap).allow);
  driver->set_axion_hook(read_policy);
  deny_new();
  driver->set_axion_hook(
      make_axion_policy_hook("(policy (require-axion-event (reason \"action=Write\")))"));
  T81_TEST_CHECK(driver->write_object(ObjectType::RawBlock, second_bytes));
  T81_TEST_CHECK(snapshot(root).size() == empty.size() + 2);

  // Both interpreted and compiled policy engines honor final authorization with
  // an honest Nop context. Normal instruction evaluation remains unchanged.
  for (bool compiled : {false, true}) {
    auto policy = parse_policy(missing_event);
    T81_TEST_CHECK(policy);
    if (compiled) policy->compile_to_bytecode();
    PolicyEngine engine(*policy);
    SyscallContext context;
    context.current_tier = 1;
    T81_TEST_CHECK(context.next_opcode == t81::tisc::Opcode::Nop);
    T81_TEST_CHECK(engine.evaluate(context).kind == VerdictKind::Allow);
    context.final_authorization = true;
    T81_TEST_CHECK(engine.evaluate(context).kind == VerdictKind::Deny);
    context.trace_reasons = {"commit-authorized"};
    T81_TEST_CHECK(engine.evaluate(context).kind == VerdictKind::Allow);
  }
  PolicyEngine no_policy(std::nullopt);
  SyscallContext final_context;
  final_context.final_authorization = true;
  T81_TEST_CHECK(no_policy.evaluate(final_context).kind == VerdictKind::Deny);
  for (bool compiled : {false, true}) {
    Policy loop_policy;
    Policy::LoopHint hint;
    hint.file = "current";
    hint.bound_value = 1;
    loop_policy.loops.push_back(hint);
    if (compiled) loop_policy.compile_to_bytecode();
    PolicyEngine engine(loop_policy);
    SyscallContext context;
    context.current_tier = 1;
    context.trace_reasons = {"loop hint file=current line=0 column=0 bound=1"};
    T81_TEST_CHECK(engine.evaluate(context).kind == VerdictKind::Allow);
    context.trace_reasons.clear();
    context.final_authorization = true;
    T81_TEST_CHECK(engine.evaluate(context).kind == VerdictKind::Deny);
  }

  // A failing engine's verdict must also prevent filesystem side effects.
  auto invalid_bytecode = t81::axion::Policy{};
  invalid_bytecode.bytecode = {0x00};
  PolicyEngine invalid_engine(invalid_bytecode);
  driver->set_axion_hook([&](auto, const auto&) {
    const auto verdict = invalid_engine.evaluate(final_context);
    return AxionVerdict{verdict.kind == VerdictKind::Allow, verdict.reason};
  });
  deny_new();

  // In-memory callback failures also fail closed (no hidden object insertion).
  auto memory = make_in_memory_driver();
  memory->set_axion_hook([](auto, const auto&) -> AxionVerdict { throw 1; });
  T81_TEST_CHECK(!memory->write_object(ObjectType::RawBlock, bytes));
  memory->set_axion_hook({});
  T81_TEST_CHECK(memory->write_object(ObjectType::RawBlock, bytes));
  auto device = std::make_unique<CountedDevice>();
  auto* counter = device.get();
  auto block = make_block_backed_driver(std::move(device), false);
  block->set_axion_hook([](auto, const auto&) -> AxionVerdict { throw 1; });
  auto block_result = block->write_object(ObjectType::RawBlock, bytes);
  T81_TEST_CHECK(!block_result && block_result.error() == Error::CapabilityError);
  T81_TEST_CHECK(counter->writes == 0);
  return 0;
}
