#include <cmath>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "test_runtime_check.hpp"

#include "t81/canonfs/canon_driver.hpp"
#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

namespace {

std::vector<std::byte> serialize_tensor(const t81::weights::NativeTensor& tensor) {
  std::vector<std::byte> buffer;
  buffer.reserve(72 + tensor.data.size() * sizeof(uint64_t));

  buffer.push_back(static_cast<std::byte>(0x20));
  buffer.push_back(static_cast<std::byte>(1));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.format)));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.shape.size())));
  for (int i = 0; i < 4; ++i) buffer.push_back(static_cast<std::byte>(0));

  for (int i = 0; i < 8; ++i) {
    uint64_t dim = (i < static_cast<int>(tensor.shape.size())) ? tensor.shape[static_cast<std::size_t>(i)] : 0;
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

t81::weights::NativeTensor canonical_fixed_native(const t81::T729DynamicTensor& tensor) {
  T81_TEST_CHECK(tensor.has_canonical_fixed_data());
  std::ostringstream payload(std::ios::binary);
  for (const auto& value : tensor.canonical_fixed_data()) {
    value.v.serialize(payload);
  }
  const std::string bytes = payload.str();
  T81_TEST_CHECK((bytes.size() % sizeof(uint64_t)) == 0U);

  t81::weights::NativeTensor native;
  native.format = t81::weights::NativeFormat::CanonicalFixed;
  native.shape.reserve(tensor.shape().size());
  for (int dim : tensor.shape()) {
    native.shape.push_back(static_cast<uint64_t>(dim));
  }
  native.trits = static_cast<uint64_t>(tensor.data().size()) *
                 static_cast<uint64_t>(t81::core::detail::DFixed::Storage::kNumTrits);
  native.data.resize(bytes.size() / sizeof(uint64_t));
  std::memcpy(native.data.data(), bytes.data(), bytes.size());
  return native;
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

}  // namespace

int main() {
  const auto old_cwd = std::filesystem::current_path();
  const auto workdir = std::filesystem::temp_directory_path() / "t81-vm-tloadhash-canonical-fixed";
  std::filesystem::remove_all(workdir);
  std::filesystem::create_directories(workdir / ".t81_canonfs");
  std::filesystem::current_path(workdir);

  const t81::T729DynamicTensor source({2}, {0.5f, -1.25f});
  const auto native = canonical_fixed_native(source);
  const auto serialized = serialize_tensor(native);

  auto driver =
      t81::canonfs::make_persistent_driver(std::filesystem::current_path() / ".t81_canonfs");
  auto write = driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                                    std::span<const std::byte>(serialized.data(), serialized.size()));
  T81_TEST_CHECK(write.has_value());

  const t81::T729DynamicTensor exact_source({3}, {-1.0f, 0.0f, 1.0f});
  const auto exact_native = canonical_fixed_native(exact_source);
  const auto exact_serialized = serialize_tensor(exact_native);
  auto exact_write = driver->write_object(
      t81::canonfs::ObjectType::CanonTensor,
      std::span<const std::byte>(exact_serialized.data(), exact_serialized.size()));
  T81_TEST_CHECK(exact_write.has_value());

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

  const auto& tensor = state.tensors[static_cast<std::size_t>(ctx.registers[2] - 1)];
  T81_TEST_CHECK(tensor.has_value());
  T81_TEST_CHECK(tensor->shape() == std::vector<int>({2}));
  T81_TEST_CHECK(tensor->numeric_class() == t81::TensorNumericClass::HostFloat);
  T81_TEST_CHECK(tensor->has_canonical_fixed_data());
  T81_TEST_CHECK(std::fabs(tensor->data()[0] - 0.5f) < 1e-4f);
  T81_TEST_CHECK(std::fabs(tensor->data()[1] + 1.25f) < 1e-4f);

  auto exact_program = make_tloadhash_program("sha3-256:" + exact_write->hash.h.to_string());
  auto exact_vm = t81::vm::make_interpreter_vm();
  exact_vm->load_program(exact_program);
  auto exact_result = exact_vm->run_to_halt();
  T81_TEST_CHECK(exact_result.has_value());

  const auto& exact_state = exact_vm->state();
  const auto& exact_ctx = exact_state.contexts[0];
  T81_TEST_CHECK(exact_ctx.register_tags[2] == t81::vm::ValueTag::TensorHandle);
  const auto& exact_tensor = exact_state.tensors[static_cast<std::size_t>(exact_ctx.registers[2] - 1)];
  T81_TEST_CHECK(exact_tensor.has_value());
  T81_TEST_CHECK(exact_tensor->numeric_class() == t81::TensorNumericClass::ExactTrit);
  T81_TEST_CHECK(exact_tensor->canonical_fixed_authoritative());
  T81_TEST_CHECK(exact_tensor->data() == exact_source.data());

  std::filesystem::current_path(old_cwd);
  std::filesystem::remove_all(workdir);
  return 0;
}
