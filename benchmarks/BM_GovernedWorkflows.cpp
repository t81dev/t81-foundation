#include <benchmark/benchmark.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

namespace {

using t81::tisc::Insn;
using t81::tisc::Opcode;
using t81::tisc::Program;

Program make_arith_chain_program(bool with_allow_policy) {
  Program p;
  p.insns.reserve(82);
  p.insns.push_back(Insn{Opcode::LoadImm, 0, 1, 0, t81::tisc::LiteralKind::Int});
  for (int i = 1; i <= 40; ++i) {
    p.insns.push_back(Insn{Opcode::LoadImm, 1, i, 0, t81::tisc::LiteralKind::Int});
    p.insns.push_back(Insn{Opcode::Add, 0, 0, 1, t81::tisc::LiteralKind::Int});
  }
  p.insns.push_back(Insn{Opcode::Halt, 0, 0, 0, t81::tisc::LiteralKind::Int});
  if (with_allow_policy) {
    p.axion_policy_text = "(policy (tier 1) (max-instructions 128))";
  }
  return p;
}

t81::weights::NativeTensor make_balanced_tensor(uint64_t elements) {
  t81::weights::NativeTensor tensor;
  const uint64_t side = std::max<uint64_t>(1, static_cast<uint64_t>(std::sqrt(static_cast<double>(elements))));
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {side, side};
  tensor.trits = side * side;
  const uint64_t limb_count = (tensor.trits + 47) / 48;
  tensor.data.reserve(static_cast<size_t>(limb_count));
  for (uint64_t i = 0; i < limb_count; ++i) {
    tensor.data.push_back((i % 2 == 0) ? 40ULL : 67ULL);
  }
  return tensor;
}

std::vector<std::byte> serialize_tensor(const t81::weights::NativeTensor& tensor) {
  std::vector<std::byte> buffer;
  buffer.reserve(72 + tensor.data.size() * 8);

  buffer.push_back(static_cast<std::byte>(0x20));
  buffer.push_back(static_cast<std::byte>(1));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.format)));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.shape.size())));
  for (int i = 0; i < 4; ++i) {
    buffer.push_back(static_cast<std::byte>(0));
  }

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

Program make_weights_load_program(uint64_t elements, bool with_allow_policy) {
  Program program;
  program.symbol_pool = {"tensorA"};
  auto model = std::make_shared<t81::weights::ModelFile>();
  model->native["tensorA"] = make_balanced_tensor(elements);
  program.weights_model = model;
  if (with_allow_policy) {
    program.axion_policy_text = "(policy (tier 1) (max-instructions 32))";
  }
  program.insns.push_back({Opcode::WeightsLoad, 1, 1, 0});
  program.insns.push_back({Opcode::Halt, 0, 0, 0});
  return program;
}

Program make_tloadhash_program(const std::string& hash_symbol, bool with_allow_policy) {
  Program program;
  program.symbol_pool.push_back(hash_symbol);
  if (with_allow_policy) {
    program.axion_policy_text =
        "(policy (tier 1) (allowed-tensor-hashes [\"" + hash_symbol + "\"]))";
  }

  Insn load_hash{Opcode::LoadImm, 1, 1, 0};
  load_hash.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  program.insns.push_back(load_hash);
  program.insns.push_back({Opcode::TLoadHash, 2, 1, 0});
  program.insns.push_back({Opcode::Halt, 0, 0, 0});
  return program;
}

std::filesystem::path governed_benchmark_root(uint64_t elements) {
  const auto pid =
#ifdef _WIN32
      static_cast<unsigned long>(_getpid());
#else
      static_cast<unsigned long>(::getpid());
#endif
  return std::filesystem::temp_directory_path() /
         ("t81-governed-bench-" + std::to_string(pid) + "-" + std::to_string(elements));
}

std::string ensure_canonfs_tensor(uint64_t elements, const std::filesystem::path& root) {
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
  auto driver = t81::canonfs::make_persistent_driver(root);
  auto bytes = serialize_tensor(make_balanced_tensor(elements));
  auto write = driver->write_object(
      t81::canonfs::ObjectType::CanonTensor,
      std::span<const std::byte>(bytes.data(), bytes.size()));
  if (!write.has_value()) {
    return {};
  }
  return "sha3-256:" + write->hash.h.to_string();
}

std::pair<std::shared_ptr<t81::canonfs::Driver>, std::string> make_in_memory_hash_fixture(
    uint64_t elements) {
  auto driver = std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_in_memory_driver().release());
  auto bytes = serialize_tensor(make_balanced_tensor(elements));
  auto write = driver->write_object(
      t81::canonfs::ObjectType::CanonTensor,
      std::span<const std::byte>(bytes.data(), bytes.size()));
  if (!write.has_value()) {
    return {nullptr, {}};
  }
  return {std::move(driver), "sha3-256:" + write->hash.h.to_string()};
}

static void BM_GovernedVMRun_Arith_NoPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(false);
  state.SetLabel("workflow=vm-run, governance=none, workload=arith-chain");
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * prog.insns.size());
}
BENCHMARK(BM_GovernedVMRun_Arith_NoPolicy)->Repetitions(3);

static void BM_GovernedVMRun_Arith_AllowPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(true);
  state.SetLabel("workflow=vm-run, governance=allow-policy, workload=arith-chain");
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * prog.insns.size());
}
BENCHMARK(BM_GovernedVMRun_Arith_AllowPolicy)->Repetitions(3);

static void BM_GovernedTensorLoad_LocalWeights_NoPolicy(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  const Program prog = make_weights_load_program(elements, false);
  state.SetLabel("workflow=tensor-load, source=local-weights, governance=none");
  state.counters["tensor_elements"] = static_cast<double>(elements);
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_GovernedTensorLoad_LocalWeights_NoPolicy)->Arg(4)->Arg(256)->Arg(4096);

static void BM_GovernedTensorLoad_LocalWeights_AllowPolicy(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  const Program prog = make_weights_load_program(elements, true);
  state.SetLabel("workflow=tensor-load, source=local-weights, governance=allow-policy");
  state.counters["tensor_elements"] = static_cast<double>(elements);
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_GovernedTensorLoad_LocalWeights_AllowPolicy)->Arg(4)->Arg(256)->Arg(4096);

static void BM_GovernedTensorLoad_CanonFSHash_NoPolicy(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  const auto root = governed_benchmark_root(elements);
  const std::string hash_symbol = ensure_canonfs_tensor(elements, root);
  if (hash_symbol.empty()) {
    state.SkipWithError("failed to write CanonFS tensor fixture");
    return;
  }
  const Program prog = make_tloadhash_program(hash_symbol, false);
  state.SetLabel("workflow=tensor-load, source=canonfs-hash, governance=none");
  state.counters["tensor_elements"] = static_cast<double>(elements);
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->set_canonfs_root(root);
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
}
BENCHMARK(BM_GovernedTensorLoad_CanonFSHash_NoPolicy)->Arg(4)->Arg(256)->Arg(4096);

static void BM_GovernedTensorLoad_CanonFSHash_AllowPolicy(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  const auto root = governed_benchmark_root(elements);
  const std::string hash_symbol = ensure_canonfs_tensor(elements, root);
  if (hash_symbol.empty()) {
    state.SkipWithError("failed to write CanonFS tensor fixture");
    return;
  }
  const Program prog = make_tloadhash_program(hash_symbol, true);
  state.SetLabel("workflow=tensor-load, source=canonfs-hash, governance=allow-policy");
  state.counters["tensor_elements"] = static_cast<double>(elements);
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->set_canonfs_root(root);
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
}
BENCHMARK(BM_GovernedTensorLoad_CanonFSHash_AllowPolicy)->Arg(4)->Arg(256)->Arg(4096);

static void BM_GovernedTensorLoad_HashFixture_NoPolicy(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto [driver, hash_symbol] = make_in_memory_hash_fixture(elements);
  if (!driver || hash_symbol.empty()) {
    state.SkipWithError("failed to write in-memory hash fixture");
    return;
  }
  const Program prog = make_tloadhash_program(hash_symbol, false);
  state.SetLabel("workflow=tensor-load, source=hash-fixture, backing=in-memory, governance=none");
  state.counters["tensor_elements"] = static_cast<double>(elements);
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->set_canonfs_driver(driver);
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_GovernedTensorLoad_HashFixture_NoPolicy)->Arg(4)->Arg(256)->Arg(4096);

static void BM_GovernedTensorLoad_HashFixture_AllowPolicy(benchmark::State& state) {
  const uint64_t elements = static_cast<uint64_t>(state.range(0));
  auto [driver, hash_symbol] = make_in_memory_hash_fixture(elements);
  if (!driver || hash_symbol.empty()) {
    state.SkipWithError("failed to write in-memory hash fixture");
    return;
  }
  const Program prog = make_tloadhash_program(hash_symbol, true);
  state.SetLabel(
      "workflow=tensor-load, source=hash-fixture, backing=in-memory, governance=allow-policy");
  state.counters["tensor_elements"] = static_cast<double>(elements);
  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->set_canonfs_driver(driver);
    vm->load_program(prog);
    auto result = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_GovernedTensorLoad_HashFixture_AllowPolicy)->Arg(4)->Arg(256)->Arg(4096);

}  // namespace
