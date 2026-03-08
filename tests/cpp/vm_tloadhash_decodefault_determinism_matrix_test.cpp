#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "vm_tloadhash_decodefault_determinism_matrix_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

std::uint64_t mix(std::uint64_t seed, std::uint64_t value) {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

std::vector<std::byte> serialize_tensor(const t81::weights::NativeTensor& tensor) {
  std::vector<std::byte> buffer;
  buffer.reserve(72 + tensor.data.size() * 8);

  buffer.push_back(static_cast<std::byte>(0x20));
  buffer.push_back(static_cast<std::byte>(1));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.format)));
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.shape.size())));
  for (int i = 0; i < 4; ++i) buffer.push_back(static_cast<std::byte>(0));

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

struct RunSummary {
  bool ok{false};
  t81::vm::Trap trap{t81::vm::Trap::None};
  std::uint64_t signature{0};
};

RunSummary run_once(const t81::tisc::Program& program,
                    const std::filesystem::path& canon_root = {}) {
  auto vm = t81::vm::make_interpreter_vm();
  if (!canon_root.empty()) vm->set_canonfs_root(canon_root);
  vm->load_program(program);
  auto run = vm->run_to_halt(256);

  RunSummary out;
  out.ok = run.has_value();
  out.trap = run.has_value() ? t81::vm::Trap::None : run.error();

  const auto& st = vm->state();
  std::uint64_t sig = 1469598103934665603ULL;
  for (const auto& tr : st.trace) {
    sig = mix(sig, tr.pc);
    sig = mix(sig, static_cast<std::uint64_t>(tr.opcode));
    sig = mix(sig, tr.trap.has_value() ? static_cast<std::uint64_t>(tr.trap.value()) : 0ULL);
  }
  for (const auto& ev : st.axion_log) {
    sig = mix(sig, static_cast<std::uint64_t>(ev.opcode));
    sig = mix(sig, static_cast<std::uint64_t>(ev.tag));
    sig = mix(sig, static_cast<std::uint64_t>(ev.value));
    for (unsigned char c : ev.verdict.reason) sig = mix(sig, c);
  }
  out.signature = sig;
  return out;
}

}  // namespace

int main() {
  const auto old_cwd = std::filesystem::current_path();
  const auto nonce = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const auto workdir =
      std::filesystem::temp_directory_path() / ("t81-vm-tloadhash-decodefault-det-" + nonce);
  std::filesystem::remove_all(workdir);
  std::filesystem::create_directories(workdir / ".t81_canonfs");
  std::filesystem::current_path(workdir);

  auto driver = t81::canonfs::make_persistent_driver(workdir / ".t81_canonfs");

  std::vector<std::byte> malformed_short = {std::byte{0x20}, std::byte{0x01}, std::byte{0x00}};
  auto write_short = driver->write_object(
      t81::canonfs::ObjectType::CanonTensor,
      std::span<const std::byte>(malformed_short.data(), malformed_short.size()));
  if (!expect(write_short.has_value(), "write malformed short tensor failed")) return 1;
  const std::string malformed_short_hash = "sha3-256:" + write_short->hash.h.to_string();

  t81::weights::NativeTensor tensor;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {2, 2};
  tensor.trits = 4;
  tensor.data = {40};
  auto malformed_rank = serialize_tensor(tensor);
  malformed_rank[3] = std::byte{9};
  auto write_rank = driver->write_object(
      t81::canonfs::ObjectType::CanonTensor,
      std::span<const std::byte>(malformed_rank.data(), malformed_rank.size()));
  if (!expect(write_rank.has_value(), "write malformed rank tensor failed")) return 1;
  const std::string malformed_rank_hash = "sha3-256:" + write_rank->hash.h.to_string();
  const std::string invalid_long_hash =
      "sha3-256:" + write_short->hash.h.to_string() + write_rank->hash.h.to_string();

  t81::weights::NativeTensor miss_tensor;
  miss_tensor.format = t81::weights::NativeFormat::BalancedTernary;
  miss_tensor.shape = {2, 2};
  miss_tensor.trits = 4;
  miss_tensor.data = {40};
  auto source_driver = t81::canonfs::make_persistent_driver(workdir / "source_canonfs");
  auto miss_serialized = serialize_tensor(miss_tensor);
  auto write_miss = source_driver->write_object(
      t81::canonfs::ObjectType::CanonTensor,
      std::span<const std::byte>(miss_serialized.data(), miss_serialized.size()));
  if (!expect(write_miss.has_value(), "write canonfs miss source tensor failed")) return 1;
  const std::string canonfs_miss_hash = "sha3-256:" + write_miss->hash.h.to_string();

  auto canonfs_miss_program = make_tloadhash_program(canonfs_miss_hash);
  canonfs_miss_program.axion_policy_text =
      "(policy (tier 1) (allowed-tensor-hashes [\"" + canonfs_miss_hash + "\"]))";

  struct MatrixCase {
    std::string id;
    t81::tisc::Program program;
    t81::vm::Trap expected_trap;
    bool expect_canonfs_miss_reason;
    bool expect_policy_violation_reason;
    bool expect_empty_allowlist_reason;
  };
  std::vector<MatrixCase> cases;
  cases.push_back({"invalid-hash-string", make_tloadhash_program(invalid_long_hash),
                   t81::vm::Trap::DecodeFault, false, false, false});
  cases.push_back(
      {"canonfs-miss", canonfs_miss_program, t81::vm::Trap::BoundsFault, true, false, false});
  t81::tisc::Program empty_allowlist_program = make_tloadhash_program(invalid_long_hash);
  empty_allowlist_program.axion_policy_text = "(policy (tier 1) (allowed-tensor-hashes []))";
  cases.push_back({"empty-allowlist-deny", empty_allowlist_program, t81::vm::Trap::SecurityFault,
                   false, false, true});

  t81::tisc::Program policy_violation_program = make_tloadhash_program(canonfs_miss_hash);
  policy_violation_program.axion_policy_text =
      "(policy (tier 1) (allowed-tensor-hashes [\"sha3-256:"
      "0000000000000000000000000000000000000000000000000000000000000000\"]))";
  cases.push_back({"policy-violation-deny", policy_violation_program,
                   t81::vm::Trap::SecurityFault, false, true, false});
  cases.push_back({"malformed-short-object", make_tloadhash_program(malformed_short_hash),
                   t81::vm::Trap::DecodeFault, false, false, false});
  cases.push_back({"malformed-rank-object", make_tloadhash_program(malformed_rank_hash),
                   t81::vm::Trap::DecodeFault, false, false, false});
  for (const auto& c : cases) {
    RunSummary baseline = run_once(c.program, workdir / ".t81_canonfs");
    if (!expect(!baseline.ok, c.id + ": expected trap")) return 1;
    if (!expect(baseline.trap == c.expected_trap, c.id + ": trap classification mismatch")) {
      return 1;
    }

    auto vm = t81::vm::make_interpreter_vm();
    vm->set_canonfs_root(workdir / ".t81_canonfs");
    vm->load_program(c.program);
    auto run = vm->run_to_halt(256);
    if (!expect(!run.has_value(), c.id + ": expected trap on classification replay")) return 1;
    const auto& events = vm->state().axion_log;
    const auto has_canonfs_miss = std::any_of(events.begin(), events.end(), [](const auto& ev) {
      return ev.verdict.reason.find("TLOADHASH canonfs_miss hash=") != std::string::npos;
    });
    if (!expect(has_canonfs_miss == c.expect_canonfs_miss_reason,
                c.id + ": canonfs-miss reason classification mismatch")) {
      return 1;
    }
    const auto has_policy_violation = std::any_of(events.begin(), events.end(), [](const auto& ev) {
      return ev.verdict.reason.find("TLOADHASH policy_violation hash=") != std::string::npos;
    });
    if (!expect(has_policy_violation == c.expect_policy_violation_reason,
                c.id + ": policy-violation reason classification mismatch")) {
      return 1;
    }
    const auto has_empty_allowlist = std::any_of(events.begin(), events.end(), [](const auto& ev) {
      return ev.verdict.reason.find("TLOADHASH denied (allowed-tensor-hashes empty)") !=
             std::string::npos;
    });
    if (!expect(has_empty_allowlist == c.expect_empty_allowlist_reason,
                c.id + ": empty-allowlist reason classification mismatch")) {
      return 1;
    }

    for (int i = 0; i < 8; ++i) {
      RunSummary repeat = run_once(c.program, workdir / ".t81_canonfs");
      if (!expect(repeat.ok == baseline.ok, c.id + ": outcome drift")) return 1;
      if (!expect(repeat.trap == baseline.trap, c.id + ": trap drift")) return 1;
      if (!expect(repeat.signature == baseline.signature, c.id + ": signature drift")) return 1;
    }
  }

  std::filesystem::current_path(old_cwd);
  std::filesystem::remove_all(workdir);
  return 0;
}
