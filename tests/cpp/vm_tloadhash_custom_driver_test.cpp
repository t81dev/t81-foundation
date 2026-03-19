#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
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

  buffer.push_back(static_cast<std::byte>(0x20));
  buffer.push_back(static_cast<std::byte>(1));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.format)));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.shape.size())));
  for (int i = 0; i < 4; ++i) buffer.push_back(static_cast<std::byte>(0));

  for (int i = 0; i < 8; ++i) {
    const uint64_t dim = (i < static_cast<int>(tensor.shape.size())) ? tensor.shape[i] : 0;
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

}  // namespace

int main() {
  t81::weights::NativeTensor tensor;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {2, 2};
  tensor.trits = 4;
  tensor.data = {40};

  auto driver = std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver().release());
  auto serialized = serialize_tensor(tensor);
  auto write =
      driver->write_object(t81::canonfs::ObjectType::CanonTensor,
                           std::span<const std::byte>(serialized.data(), serialized.size()));
  T81_TEST_CHECK(write.has_value());

  const std::string hash_symbol = "sha3-256:" + write->hash.h.to_string();
  auto program = make_tloadhash_program(hash_symbol);

  auto vm = t81::vm::make_interpreter_vm();
  vm->set_canonfs_driver(driver);
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

  const auto has_success_trace =
      std::any_of(state.axion_log.begin(), state.axion_log.end(), [&](const auto& event) {
        return event.verdict.reason.find("TLOADHASH success hash=" + hash_symbol) !=
               std::string::npos;
      });
  T81_TEST_CHECK(has_success_trace);
  return 0;
}
