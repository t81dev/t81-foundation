#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

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
// Retain real device bytes so tests observe effects on a populated store, not
// just return codes. Counters also expose rejected flushes and index rebuilds.
class CountedDevice final : public t81::ternaryos::dev::IBlockDevice {
public:
  std::array<t81::ternaryos::dev::BlockData, 16> blocks{};
  mutable std::size_t reads{0};
  std::size_t writes{0};
  std::size_t flushes{0};
  t81::ternaryos::dev::BlockDeviceInfo info() const noexcept override { return {}; }
  uint64_t block_count() const noexcept override { return blocks.size(); }
  bool read_block(uint64_t lba, t81::ternaryos::dev::BlockData& out) const override {
    ++reads;
    if (lba >= blocks.size()) return false;
    out = blocks[lba];
    return true;
  }
  bool write_block(uint64_t lba, const t81::ternaryos::dev::BlockData& data) override {
    ++writes;
    if (lba >= blocks.size()) return false;
    blocks[lba] = data;
    return true;
  }
  bool flush() override {
    ++flushes;
    return true;
  }
};

enum class HookMode { Deny, Throw, MissingPolicy, InvalidPolicy, Allow, Bootstrap };

void block_metadata_case(OpKind operation, HookMode mode) {
  auto device = std::make_unique<CountedDevice>();
  auto* counter = device.get();
  auto driver = make_block_backed_driver(std::move(device), false);
  const std::string payload = "seeded block policy boundary";
  const auto bytes = std::as_bytes(std::span(payload.data(), payload.size()));
  const auto seed = driver->write_object(ObjectType::RawBlock, bytes);
  T81_TEST_CHECK(seed);
  CapabilityGrant grant{};
  grant.target = *seed;
  grant.perms = CANON_PERM_READ;
  T81_TEST_CHECK(driver->publish_capability(grant));

  // The published grant follows the seeded payload at LBA 2. Derive its ref
  // from the stored block without duplicating the driver's serialization.
  CanonBlock capability_block{};
  capability_block.trytes = counter->blocks[2];
  const CanonRef capability_ref{capability_block.hash()};
  const auto capability_bytes = driver->read_object_bytes(capability_ref);
  T81_TEST_CHECK(capability_bytes && capability_bytes->size() == 106);
  grant.perms = CANON_PERM_READ | CANON_PERM_WRITE;  // Publish must not deduplicate.
  const CanonRef target = operation == OpKind::Revoke ? capability_ref : *seed;

  const auto before_blocks = counter->blocks;
  const auto before_reads = counter->reads;
  const auto before_writes = counter->writes;
  const auto before_flushes = counter->flushes;
  std::size_t calls = 0;
  auto production_hook = make_axion_policy_hook(mode == HookMode::MissingPolicy   ? ""
                                                : mode == HookMode::InvalidPolicy ? "invalid-policy"
                                                                                  : allow);
  driver->set_axion_hook([&](OpKind kind, const CanonRef& ref) -> AxionVerdict {
    ++calls;
    T81_TEST_CHECK(kind == operation);
    T81_TEST_CHECK(ref.hash == target.hash);
    // Authorization precedes even index reads/flushes, not just payload writes.
    T81_TEST_CHECK(counter->blocks == before_blocks);
    T81_TEST_CHECK(counter->reads == before_reads);
    T81_TEST_CHECK(counter->writes == before_writes);
    T81_TEST_CHECK(counter->flushes == before_flushes);
    if (mode == HookMode::Throw) throw std::runtime_error("metadata hook failed");
    if (mode == HookMode::Deny) return AxionVerdict{false, "explicit deny"};
    return production_hook(kind, ref);
  });
  if (mode == HookMode::Bootstrap) driver->set_axion_hook({});

  const auto result = [&]() -> Result<void> {
    switch (operation) {
      case OpKind::Publish:
        return driver->publish_capability(grant);
      case OpKind::Revoke:
        return driver->revoke_capability(capability_ref);
      case OpKind::Repair:
        return driver->parity_repair_subtree(*seed);
      default:
        throw std::runtime_error("unexpected metadata operation");
    }
  }();
  T81_TEST_CHECK(calls == (mode == HookMode::Bootstrap ? 0u : 1u));
  const bool permitted = mode == HookMode::Allow || mode == HookMode::Bootstrap;
  if (!permitted) {
    T81_TEST_CHECK(!result && result.error() == Error::CapabilityError);
    T81_TEST_CHECK(counter->blocks == before_blocks);
    T81_TEST_CHECK(counter->reads == before_reads);
    T81_TEST_CHECK(counter->writes == before_writes);
    T81_TEST_CHECK(counter->flushes == before_flushes);
  } else {
    T81_TEST_CHECK(result);
    T81_TEST_CHECK(counter->reads > before_reads);
    if (operation == OpKind::Repair) {
      T81_TEST_CHECK(counter->blocks == before_blocks);
      T81_TEST_CHECK(counter->writes == before_writes);
      T81_TEST_CHECK(counter->flushes == before_flushes);
    } else {
      T81_TEST_CHECK(counter->blocks != before_blocks);
      T81_TEST_CHECK(counter->writes > before_writes);
      T81_TEST_CHECK(counter->flushes == before_flushes + 1);
    }
  }

  driver->set_axion_hook({});
  const auto reads_before_verify = counter->reads;
  const auto surviving_seed = driver->read_object_bytes(*seed);
  T81_TEST_CHECK(surviving_seed);
  T81_TEST_CHECK(*surviving_seed == std::vector<std::byte>(bytes.begin(), bytes.end()));
  T81_TEST_CHECK(counter->reads == reads_before_verify + 1);
  const auto surviving_capability = driver->read_object_bytes(capability_ref);
  if (permitted && operation == OpKind::Revoke) {
    T81_TEST_CHECK(!surviving_capability && surviving_capability.error() == Error::NotFound);
    T81_TEST_CHECK(counter->reads == reads_before_verify + 1);  // Removed from index.
  } else {
    T81_TEST_CHECK(surviving_capability && *surviving_capability == *capability_bytes);
    T81_TEST_CHECK(counter->reads == reads_before_verify + 2);
  }
  if (permitted && operation == OpKind::Publish) {
    CanonBlock published{};
    published.trytes = counter->blocks[3];
    const auto stored_grant = driver->read_object_bytes(CanonRef{published.hash()});
    T81_TEST_CHECK(stored_grant && stored_grant->size() == 106);
    T81_TEST_CHECK(*stored_grant != *capability_bytes);
  }
}

void block_metadata_policy_matrix() {
  for (const auto operation : {OpKind::Publish, OpKind::Revoke, OpKind::Repair}) {
    for (const auto mode : {HookMode::Deny, HookMode::Throw, HookMode::MissingPolicy,
                            HookMode::InvalidPolicy, HookMode::Allow, HookMode::Bootstrap}) {
      block_metadata_case(operation, mode);
    }
  }
  std::cout << "block metadata policy matrix: 12 negative cases and 6 positive controls passed\n";
}
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
  block_metadata_policy_matrix();
  return 0;
}
