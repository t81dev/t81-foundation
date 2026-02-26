#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "test_runtime_check.hpp"

#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

namespace {

std::vector<std::byte> serialize_tensor(const t81::weights::NativeTensor& tensor) {
  std::vector<std::byte> buffer;
  buffer.reserve(72 + tensor.data.size() * 8);

  buffer.push_back(static_cast<std::byte>(0x20));  // type
  buffer.push_back(static_cast<std::byte>(1));     // version
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.format)));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.shape.size())));
  for (int i = 0; i < 4; ++i) {
    buffer.push_back(static_cast<std::byte>(0));
  }

  for (int i = 0; i < 8; ++i) {
    uint64_t dim = (i < static_cast<int>(tensor.shape.size())) ? tensor.shape[i] : 0;
    for (int b = 0; b < 8; ++b) {
      buffer.push_back(static_cast<std::byte>((dim >> (b * 8)) & 0xFF));
    }
  }

  for (uint64_t val : tensor.data) {
    for (int b = 0; b < 8; ++b) {
      buffer.push_back(static_cast<std::byte>((val >> (b * 8)) & 0xFF));
    }
  }
  return buffer;
}

t81::tisc::Program make_tloadhash_program(const std::string& hash_symbol) {
  t81::tisc::Program program;
  program.symbol_pool.push_back(hash_symbol);

  t81::tisc::Insn load_hash{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_hash.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  program.insns.push_back(load_hash);
  program.insns.push_back({t81::tisc::Opcode::TLoadHash, 2, 1, 0});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return program;
}

void run_tloadhash_success_case() {
  // Packed digits for 4 trits all equal to 0 (balanced): ternary digits [1,1,1,1] => 40.
  t81::weights::NativeTensor tensor;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {2, 2};
  tensor.trits = 4;
  tensor.data = {40};

  auto driver =
      t81::canonfs::make_persistent_driver(std::filesystem::current_path() / ".t81_canonfs");
  auto serialized = serialize_tensor(tensor);
  auto write =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                           std::span<const std::byte>(serialized.data(), serialized.size()));
  T81_TEST_CHECK(write.has_value());

  std::string hash_symbol = "sha3-256:" + write->hash.h.to_string();
  auto program = make_tloadhash_program(hash_symbol);

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto& state = vm->state();
  const auto& ctx = state.contexts[0];
  T81_TEST_CHECK(ctx.register_tags[2] == t81::vm::ValueTag::TensorHandle);
  T81_TEST_CHECK(ctx.registers[2] > 0);
  const auto tensor_idx = static_cast<std::size_t>(ctx.registers[2] - 1);
  T81_TEST_CHECK(tensor_idx < state.tensors.size());
  T81_TEST_CHECK(state.tensors[tensor_idx].has_value());
  T81_TEST_CHECK(state.tensors[tensor_idx]->shape().size() == 2);
  T81_TEST_CHECK(state.tensors[tensor_idx]->shape()[0] == 2);
  T81_TEST_CHECK(state.tensors[tensor_idx]->shape()[1] == 2);
  T81_TEST_CHECK(state.tensors[tensor_idx]->data().size() == 4);

  const auto has_success_trace =
      std::any_of(state.axion_log.begin(), state.axion_log.end(), [&](const auto& event) {
        return event.verdict.reason.find("TLOADHASH success hash=" + hash_symbol) !=
               std::string::npos;
      });
  T81_TEST_CHECK(has_success_trace);
}

void run_tloadhash_decode_fault_case() {
  std::vector<std::byte> malformed = {std::byte{0x20}, std::byte{0x01}, std::byte{0x00}};

  auto driver =
      t81::canonfs::make_persistent_driver(std::filesystem::current_path() / ".t81_canonfs");
  auto write = driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                                    std::span<const std::byte>(malformed.data(), malformed.size()));
  T81_TEST_CHECK(write.has_value());

  std::string hash_symbol = "sha3-256:" + write->hash.h.to_string();
  auto program = make_tloadhash_program(hash_symbol);

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::DecodeFault);
}

void run_tloadhash_ambiguous_payload_fail_closed_case() {
  t81::weights::NativeTensor tensor;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {1};
  tensor.trits = 1;
  tensor.data = {1};

  auto driver =
      t81::canonfs::make_persistent_driver(std::filesystem::current_path() / ".t81_canonfs");
  auto serialized = serialize_tensor(tensor);
  auto write =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                           std::span<const std::byte>(serialized.data(), serialized.size()));
  T81_TEST_CHECK(write.has_value());

  std::string hash_symbol = "sha3-256:" + write->hash.h.to_string();
  auto program = make_tloadhash_program(hash_symbol);

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::DecodeFault);
}

void run_tloadhash_invalid_header_cases() {
  t81::weights::NativeTensor base;
  base.format = t81::weights::NativeFormat::BalancedTernary;
  base.shape = {2, 2};
  base.trits = 4;
  base.data = {40};

  auto malformed_format = serialize_tensor(base);
  malformed_format[2] = std::byte{0x7F};

  auto malformed_rank = serialize_tensor(base);
  malformed_rank[3] = std::byte{9};

  auto driver =
      t81::canonfs::make_persistent_driver(std::filesystem::current_path() / ".t81_canonfs");

  {
    auto write = driver->write_object(
        t81::canonfs::ObjectType::CanonTensor,
        std::span<const std::byte>(malformed_format.data(), malformed_format.size()));
    T81_TEST_CHECK(write.has_value());
    std::string hash_symbol = "sha3-256:" + write->hash.h.to_string();
    auto program = make_tloadhash_program(hash_symbol);
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    T81_TEST_CHECK(!result.has_value());
    T81_TEST_CHECK(result.error() == t81::vm::Trap::DecodeFault);
  }

  {
    auto write = driver->write_object(
        t81::canonfs::ObjectType::CanonTensor,
        std::span<const std::byte>(malformed_rank.data(), malformed_rank.size()));
    T81_TEST_CHECK(write.has_value());
    std::string hash_symbol = "sha3-256:" + write->hash.h.to_string();
    auto program = make_tloadhash_program(hash_symbol);
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    T81_TEST_CHECK(!result.has_value());
    T81_TEST_CHECK(result.error() == t81::vm::Trap::DecodeFault);
  }
}

void run_tloadhash_canonfs_miss_case() {
  t81::weights::NativeTensor tensor;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {2, 2};
  tensor.trits = 4;
  tensor.data = {40};

  // Write to an external CanonFS store so the hash exists but not in VM's store.
  auto source_driver =
      t81::canonfs::make_persistent_driver(std::filesystem::current_path() / "source_canonfs");
  auto serialized = serialize_tensor(tensor);
  auto write =
      source_driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                                  std::span<const std::byte>(serialized.data(), serialized.size()));
  T81_TEST_CHECK(write.has_value());

  std::string hash_symbol = "sha3-256:" + write->hash.h.to_string();
  auto program = make_tloadhash_program(hash_symbol);
  program.axion_policy_text = "(policy (tier 1) (allowed-tensor-hashes [\"" + hash_symbol + "\"]))";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::BoundsFault);

  const auto& state = vm->state();
  const auto has_miss_trace =
      std::any_of(state.axion_log.begin(), state.axion_log.end(), [&](const auto& event) {
        return event.verdict.reason.find("TLOADHASH canonfs_miss hash=" + hash_symbol) !=
               std::string::npos;
      });
  T81_TEST_CHECK(has_miss_trace);
}

void run_tloadhash_policy_deny_case() {
  auto program = make_tloadhash_program("sha3-256:invalid-placeholder");
  program.axion_policy_text = "(policy (tier 1) (allowed-tensor-hashes []))";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  const auto& state = vm->state();
  const auto has_empty_allowlist_deny =
      std::any_of(state.axion_log.begin(), state.axion_log.end(), [](const auto& event) {
        return event.verdict.reason.find("TLOADHASH denied (allowed-tensor-hashes empty)") !=
               std::string::npos;
      });
  T81_TEST_CHECK(has_empty_allowlist_deny);
}

void run_tloadhash_policy_violation_case() {
  t81::weights::NativeTensor tensor;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {2, 2};
  tensor.trits = 4;
  tensor.data = {40};

  auto source_driver =
      t81::canonfs::make_persistent_driver(std::filesystem::current_path() / "source_canonfs");
  auto serialized = serialize_tensor(tensor);
  auto write =
      source_driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                                  std::span<const std::byte>(serialized.data(), serialized.size()));
  T81_TEST_CHECK(write.has_value());

  std::string requested_hash = "sha3-256:" + write->hash.h.to_string();
  auto program = make_tloadhash_program(requested_hash);
  program.axion_policy_text =
      "(policy (tier 1) (allowed-tensor-hashes [\"sha3-256:"
      "000000000000000000000000000000000000000000000000000000000000000000000000000000000\"]))";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == t81::vm::Trap::SecurityFault);

  const auto& state = vm->state();
  const auto has_policy_violation =
      std::any_of(state.axion_log.begin(), state.axion_log.end(), [&](const auto& event) {
        return event.verdict.reason.find("TLOADHASH policy_violation hash=" + requested_hash) !=
               std::string::npos;
      });
  T81_TEST_CHECK(has_policy_violation);
}

}  // namespace

int main() {
  const auto old_cwd = std::filesystem::current_path();
  const auto nonce = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const auto workdir = std::filesystem::temp_directory_path() / ("t81-vm-tloadhash-" + nonce);

  std::filesystem::remove_all(workdir);
  std::filesystem::create_directories(workdir / ".t81_canonfs");
  std::filesystem::current_path(workdir);

  run_tloadhash_canonfs_miss_case();
  run_tloadhash_policy_deny_case();
  run_tloadhash_policy_violation_case();
  run_tloadhash_success_case();
  run_tloadhash_decode_fault_case();
  run_tloadhash_ambiguous_payload_fail_closed_case();
  run_tloadhash_invalid_header_cases();

  std::filesystem::current_path(old_cwd);
  std::filesystem::remove_all(workdir);
  return 0;
}
