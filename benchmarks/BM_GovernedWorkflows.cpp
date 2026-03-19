#include <benchmark/benchmark.h>

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <cstdlib>
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

std::filesystem::path governed_emit_root(std::string_view name) {
  const auto pid =
#ifdef _WIN32
      static_cast<unsigned long>(_getpid());
#else
      static_cast<unsigned long>(::getpid());
#endif
  return std::filesystem::temp_directory_path() /
         ("t81-governed-emit-" + std::to_string(pid) + "-" + std::string(name));
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

std::size_t mix_hash(std::size_t seed, std::size_t value) {
  seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  return seed;
}

std::size_t hash_text(std::string_view text) {
  return std::hash<std::string_view>{}(text);
}

std::string_view verdict_kind_name(t81::axion::VerdictKind kind) {
  switch (kind) {
    case t81::axion::VerdictKind::Allow:
      return "allow";
    case t81::axion::VerdictKind::Deny:
      return "deny";
    case t81::axion::VerdictKind::Defer:
      return "defer";
    case t81::axion::VerdictKind::Warn:
      return "warn";
    case t81::axion::VerdictKind::Quarantine:
      return "quarantine";
  }
  return "unknown";
}

std::size_t materialize_observability_signature(const t81::vm::State& state) {
  std::size_t signature = 0;
  signature = mix_hash(signature, state.trace.size());
  signature = mix_hash(signature, state.axion_log.size());
  for (const auto& entry : state.trace) {
    signature = mix_hash(signature, entry.pc);
    signature = mix_hash(signature, static_cast<std::size_t>(entry.opcode));
    signature = mix_hash(signature, hash_text(t81::tisc::opcode_name(entry.opcode)));
    signature = mix_hash(signature,
                         entry.trap.has_value() ? static_cast<std::size_t>(*entry.trap) + 1U : 0U);
  }
  for (const auto& event : state.axion_log) {
    signature = mix_hash(signature, static_cast<std::size_t>(event.opcode));
    signature = mix_hash(signature, static_cast<std::size_t>(event.tag));
    signature = mix_hash(signature, static_cast<std::size_t>(event.value));
    signature = mix_hash(signature, hash_text(verdict_kind_name(event.verdict.kind)));
    signature = mix_hash(signature, hash_text(event.verdict.reason));
    signature = mix_hash(signature, hash_text(event.structured.decision));
    signature = mix_hash(signature, hash_text(event.structured.event_type));
    signature = mix_hash(signature, hash_text(event.structured.reason));
    signature = mix_hash(signature, hash_text(event.structured.reason_code));
    signature = mix_hash(signature, static_cast<std::size_t>(event.structured.policy_id));
    signature = mix_hash(signature, static_cast<std::size_t>(event.structured.pc));
    signature = mix_hash(signature, static_cast<std::size_t>(event.structured.handle_id));
  }
  return signature;
}

std::string render_trace_text(const t81::vm::State& state) {
  std::ostringstream out;
  for (const auto& entry : state.trace) {
    out << "PC=" << entry.pc << ' ' << t81::tisc::opcode_name(entry.opcode);
    if (entry.trap) {
      out << " trap=" << t81::vm::to_string(*entry.trap);
    }
    out << '\n';
  }
  return out.str();
}

std::string render_axion_audit_json(const t81::vm::State& state) {
  std::ostringstream out;
  out << "{\"schema\":\"t81.axion-events.v1\",\"events\":[";
  for (std::size_t i = 0; i < state.axion_log.size(); ++i) {
    const auto& event = state.axion_log[i];
    if (i != 0) out << ',';
    out << "{\"opcode\":\"" << t81::tisc::opcode_name(event.opcode) << "\""
        << ",\"decision\":\"" << event.structured.decision << "\""
        << ",\"verdict\":\"" << verdict_kind_name(event.verdict.kind) << "\""
        << ",\"reason\":\"" << event.verdict.reason << "\""
        << ",\"canonical_reason\":\"" << event.structured.to_canonical_reason_string() << "\""
        << ",\"event_type\":\"" << event.structured.event_type << "\""
        << ",\"reason_code\":\"" << event.structured.reason_code << "\""
        << ",\"pc\":" << event.structured.pc
        << ",\"policy_id\":" << event.structured.policy_id
        << ",\"handle_id\":" << event.structured.handle_id
        << "}";
  }
  out << "]}";
  return out.str();
}

bool write_text_file(const std::filesystem::path& path, std::string_view text) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
  out.flush();
  return static_cast<bool>(out);
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string trim_ascii_whitespace(std::string text) {
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
    text.erase(text.begin());
  }
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
    text.pop_back();
  }
  return text;
}

std::string shell_quote(const std::filesystem::path& path) {
  std::string s = path.string();
  std::string out = "'";
  for (char c : s) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out += "'";
  return out;
}

std::string shell_quote_text(std::string_view text) {
  std::string out = "'";
  for (char c : text) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out += "'";
  return out;
}

int run_shell_command(const std::string& command) {
  return std::system(command.c_str());
}

void write_small_weights_model(const std::filesystem::path& model_path, bool alternate = false) {
  t81::weights::NativeModel model;

  t81::weights::NativeTensor mat_a;
  mat_a.shape = {2, 2};
  mat_a.trits = 4;
  mat_a.data = {alternate ? 67ULL : 40ULL};
  model["mat_a"] = mat_a;

  t81::weights::NativeTensor mat_b;
  mat_b.shape = {2, 2};
  mat_b.trits = 4;
  mat_b.data = {alternate ? 40ULL : 67ULL};
  model["mat_b"] = mat_b;

  t81::weights::save_t81w(model, model_path);
}

std::string prepare_cli_weights_model_hash(const std::filesystem::path& t81_bin,
                                           const std::filesystem::path& canonfs_root,
                                           const std::filesystem::path& model_path,
                                           const std::filesystem::path& hash_out_path,
                                           bool alternate = false) {
  write_small_weights_model(model_path, alternate);
  const std::string put_cmd = shell_quote(t81_bin) + " canonfs put-file " + shell_quote(model_path) +
                              " --canonfs-root " + shell_quote(canonfs_root) + " >" +
                              shell_quote(hash_out_path) + " 2>/dev/null";
  if (run_shell_command(put_cmd) != 0) {
    return {};
  }
  const std::string model_hash = trim_ascii_whitespace(read_text_file(hash_out_path));
  if (model_hash.rfind("sha3-256:", 0) != 0) {
    return {};
  }
  return model_hash;
}

bool write_cli_weights_policy(const std::filesystem::path& policy_path, std::string_view model_checksum) {
  std::ostringstream policy;
  policy << "(policy\n"
         << "  (tier 1)\n"
         << "  (max-instructions 128)\n"
         << "  (allowed-ternary-model-hashes [\"sha3-512:" << model_checksum << "\"]))\n";
  return write_text_file(policy_path, policy.str());
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

static void BM_GovernedObservability_Arith_NoPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(false);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto run = vm->run_to_halt(10000);
  if (!run.has_value()) {
    state.SkipWithError("failed to build no-policy observability snapshot");
    return;
  }
  const auto& snapshot = vm->state();
  state.SetLabel("workflow=observability, source=vm-state, governance=none, workload=arith-chain");
  state.counters["trace_entries"] = static_cast<double>(snapshot.trace.size());
  state.counters["axion_events"] = static_cast<double>(snapshot.axion_log.size());
  for (auto _ : state) {
    auto signature = materialize_observability_signature(snapshot);
    benchmark::DoNotOptimize(signature);
  }
}
BENCHMARK(BM_GovernedObservability_Arith_NoPolicy);

static void BM_GovernedObservability_Arith_AllowPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(true);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto run = vm->run_to_halt(10000);
  if (!run.has_value()) {
    state.SkipWithError("failed to build allow-policy observability snapshot");
    return;
  }
  const auto& snapshot = vm->state();
  state.SetLabel(
      "workflow=observability, source=vm-state, governance=allow-policy, workload=arith-chain");
  state.counters["trace_entries"] = static_cast<double>(snapshot.trace.size());
  state.counters["axion_events"] = static_cast<double>(snapshot.axion_log.size());
  for (auto _ : state) {
    auto signature = materialize_observability_signature(snapshot);
    benchmark::DoNotOptimize(signature);
  }
}
BENCHMARK(BM_GovernedObservability_Arith_AllowPolicy);

static void BM_GovernedRender_Arith_NoPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(false);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto run = vm->run_to_halt(10000);
  if (!run.has_value()) {
    state.SkipWithError("failed to build no-policy render snapshot");
    return;
  }
  const auto& snapshot = vm->state();
  const auto trace_bytes = render_trace_text(snapshot).size();
  const auto audit_bytes = render_axion_audit_json(snapshot).size();
  state.SetLabel("workflow=render, source=vm-state, governance=none, workload=arith-chain");
  state.counters["trace_entries"] = static_cast<double>(snapshot.trace.size());
  state.counters["axion_events"] = static_cast<double>(snapshot.axion_log.size());
  state.counters["trace_bytes"] = static_cast<double>(trace_bytes);
  state.counters["audit_bytes"] = static_cast<double>(audit_bytes);
  for (auto _ : state) {
    auto trace_text = render_trace_text(snapshot);
    auto audit_json = render_axion_audit_json(snapshot);
    benchmark::DoNotOptimize(trace_text);
    benchmark::DoNotOptimize(audit_json);
  }
}
BENCHMARK(BM_GovernedRender_Arith_NoPolicy);

static void BM_GovernedRender_Arith_AllowPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(true);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto run = vm->run_to_halt(10000);
  if (!run.has_value()) {
    state.SkipWithError("failed to build allow-policy render snapshot");
    return;
  }
  const auto& snapshot = vm->state();
  const auto trace_bytes = render_trace_text(snapshot).size();
  const auto audit_bytes = render_axion_audit_json(snapshot).size();
  state.SetLabel("workflow=render, source=vm-state, governance=allow-policy, workload=arith-chain");
  state.counters["trace_entries"] = static_cast<double>(snapshot.trace.size());
  state.counters["axion_events"] = static_cast<double>(snapshot.axion_log.size());
  state.counters["trace_bytes"] = static_cast<double>(trace_bytes);
  state.counters["audit_bytes"] = static_cast<double>(audit_bytes);
  for (auto _ : state) {
    auto trace_text = render_trace_text(snapshot);
    auto audit_json = render_axion_audit_json(snapshot);
    benchmark::DoNotOptimize(trace_text);
    benchmark::DoNotOptimize(audit_json);
  }
}
BENCHMARK(BM_GovernedRender_Arith_AllowPolicy);

static void BM_GovernedEmit_Arith_NoPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(false);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto run = vm->run_to_halt(10000);
  if (!run.has_value()) {
    state.SkipWithError("failed to build no-policy emit snapshot");
    return;
  }
  const auto& snapshot = vm->state();
  const auto trace_text = render_trace_text(snapshot);
  const auto audit_json = render_axion_audit_json(snapshot);
  const auto root = governed_emit_root("no-policy");
  std::error_code ec;
  std::filesystem::create_directories(root, ec);
  const auto trace_path = root / "run.trace";
  const auto audit_path = root / "audit.json";
  state.SetLabel("workflow=emit, source=vm-state, governance=none, workload=arith-chain");
  state.counters["trace_entries"] = static_cast<double>(snapshot.trace.size());
  state.counters["axion_events"] = static_cast<double>(snapshot.axion_log.size());
  state.counters["trace_bytes"] = static_cast<double>(trace_text.size());
  state.counters["audit_bytes"] = static_cast<double>(audit_json.size());
  for (auto _ : state) {
    const bool trace_ok = write_text_file(trace_path, trace_text);
    const bool audit_ok = write_text_file(audit_path, audit_json);
    benchmark::DoNotOptimize(trace_ok ? 1 : 0);
    benchmark::DoNotOptimize(audit_ok ? 1 : 0);
    if (!trace_ok || !audit_ok) {
      state.SkipWithError("failed to emit no-policy trace/audit files");
      break;
    }
  }
  std::filesystem::remove_all(root, ec);
}
BENCHMARK(BM_GovernedEmit_Arith_NoPolicy);

static void BM_GovernedEmit_Arith_AllowPolicy(benchmark::State& state) {
  const Program prog = make_arith_chain_program(true);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto run = vm->run_to_halt(10000);
  if (!run.has_value()) {
    state.SkipWithError("failed to build allow-policy emit snapshot");
    return;
  }
  const auto& snapshot = vm->state();
  const auto trace_text = render_trace_text(snapshot);
  const auto audit_json = render_axion_audit_json(snapshot);
  const auto root = governed_emit_root("allow-policy");
  std::error_code ec;
  std::filesystem::create_directories(root, ec);
  const auto trace_path = root / "run.trace";
  const auto audit_path = root / "audit.json";
  state.SetLabel("workflow=emit, source=vm-state, governance=allow-policy, workload=arith-chain");
  state.counters["trace_entries"] = static_cast<double>(snapshot.trace.size());
  state.counters["axion_events"] = static_cast<double>(snapshot.axion_log.size());
  state.counters["trace_bytes"] = static_cast<double>(trace_text.size());
  state.counters["audit_bytes"] = static_cast<double>(audit_json.size());
  for (auto _ : state) {
    const bool trace_ok = write_text_file(trace_path, trace_text);
    const bool audit_ok = write_text_file(audit_path, audit_json);
    benchmark::DoNotOptimize(trace_ok ? 1 : 0);
    benchmark::DoNotOptimize(audit_ok ? 1 : 0);
    if (!trace_ok || !audit_ok) {
      state.SkipWithError("failed to emit allow-policy trace/audit files");
      break;
    }
  }
  std::filesystem::remove_all(root, ec);
}
BENCHMARK(BM_GovernedEmit_Arith_AllowPolicy);

static void BM_GovernedCLI_VMTrace_Export(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-vm-trace");
  const auto artifact = workdir / "hello.tisc";
  const auto trace_out = workdir / "hello.trace";
  std::error_code ec;
  std::filesystem::create_directories(workdir, ec);
  const std::string build_cmd =
      shell_quote(t81_bin) + " code build " + shell_quote(repo_root / "examples" / "hello_world.t81") +
      " -o " + shell_quote(artifact) + " >/dev/null";
  if (run_shell_command(build_cmd) != 0) {
    state.SkipWithError("failed to prepare CLI trace artifact");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  state.SetLabel("workflow=cli-export, command=vm-trace, governance=artifact-run");
  for (auto _ : state) {
    const std::string cmd =
        shell_quote(t81_bin) + " vm trace " + shell_quote(artifact) + " -o " + shell_quote(trace_out) +
        " >/dev/null 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 vm trace failed");
      break;
    }
  }
  if (std::filesystem::exists(trace_out, ec)) {
    state.counters["trace_bytes"] = static_cast<double>(std::filesystem::file_size(trace_out, ec));
  }
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_VMTrace_Export);

static void BM_GovernedCLI_VMTrace_Export_Accumulator(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-vm-trace-accumulator");
  const auto artifact = workdir / "accumulator.tisc";
  const auto trace_out = workdir / "accumulator.trace";
  std::error_code ec;
  std::filesystem::create_directories(workdir, ec);
  const std::string build_cmd =
      shell_quote(t81_bin) + " code build " +
      shell_quote(repo_root / "examples" / "system-integration" / "accumulator.t81") +
      " -o " + shell_quote(artifact) + " >/dev/null";
  if (run_shell_command(build_cmd) != 0) {
    state.SkipWithError("failed to prepare accumulator CLI trace artifact");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  state.SetLabel("workflow=cli-export, command=vm-trace, governance=artifact-run, workload=accumulator");
  for (auto _ : state) {
    const std::string cmd =
        shell_quote(t81_bin) + " vm trace " + shell_quote(artifact) + " -o " + shell_quote(trace_out) +
        " >/dev/null 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 vm trace accumulator failed");
      break;
    }
  }
  if (std::filesystem::exists(trace_out, ec)) {
    state.counters["trace_bytes"] = static_cast<double>(std::filesystem::file_size(trace_out, ec));
  }
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_VMTrace_Export_Accumulator);

static void BM_GovernedCLI_VMTrace_Export_SystemIntegration(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-vm-trace-system-integration");
  const auto artifact = workdir / "system-integration.tisc";
  const auto trace_out = workdir / "system-integration.trace";
  std::error_code ec;
  std::filesystem::create_directories(workdir, ec);
  const std::string build_cmd =
      shell_quote(t81_bin) + " code build " +
      shell_quote(repo_root / "examples" / "system_integration.t81") +
      " -o " + shell_quote(artifact) + " >/dev/null";
  if (run_shell_command(build_cmd) != 0) {
    state.SkipWithError("failed to prepare system-integration CLI trace artifact");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  state.SetLabel(
      "workflow=cli-export, command=vm-trace, governance=artifact-run, workload=system-integration");
  for (auto _ : state) {
    const std::string cmd =
        shell_quote(t81_bin) + " vm trace " + shell_quote(artifact) + " -o " + shell_quote(trace_out) +
        " >/dev/null 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 vm trace system-integration failed");
      break;
    }
  }
  if (std::filesystem::exists(trace_out, ec)) {
    state.counters["trace_bytes"] = static_cast<double>(std::filesystem::file_size(trace_out, ec));
  }
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_VMTrace_Export_SystemIntegration);

static void BM_GovernedCLI_VMTrace_Export_WithPolicy(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-vm-trace-policy");
  const auto artifact = workdir / "hello-policy.tisc";
  const auto trace_out = workdir / "hello-policy.trace";
  const auto policy = repo_root / "examples" / "system_integration.apl";
  std::error_code ec;
  std::filesystem::create_directories(workdir, ec);
  const std::string build_cmd =
      shell_quote(t81_bin) + " code build " + shell_quote(repo_root / "examples" / "hello_world.t81") +
      " -o " + shell_quote(artifact) + " >/dev/null";
  if (run_shell_command(build_cmd) != 0) {
    state.SkipWithError("failed to prepare policy CLI trace artifact");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  state.SetLabel("workflow=cli-export, command=vm-trace, governance=policy-file");
  for (auto _ : state) {
    const std::string cmd = shell_quote(t81_bin) + " vm trace " + shell_quote(artifact) +
                            " --policy " + shell_quote(policy) + " -o " + shell_quote(trace_out) +
                            " >/dev/null 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 vm trace with policy failed");
      break;
    }
  }
  if (std::filesystem::exists(trace_out, ec)) {
    state.counters["trace_bytes"] = static_cast<double>(std::filesystem::file_size(trace_out, ec));
  }
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_VMTrace_Export_WithPolicy);

static void BM_GovernedCLI_VMTrace_Export_NeuralNet(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-vm-trace-neural-net");
  const auto artifact = workdir / "neural-net.tisc";
  const auto trace_out = workdir / "neural-net.trace";
  std::error_code ec;
  std::filesystem::create_directories(workdir, ec);
  const std::string build_cmd =
      shell_quote(t81_bin) + " code build " + shell_quote(repo_root / "examples" / "neural_net.t81") +
      " -o " + shell_quote(artifact) + " >/dev/null";
  if (run_shell_command(build_cmd) != 0) {
    state.SkipWithError("failed to prepare neural-net CLI trace artifact");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  state.SetLabel(
      "workflow=cli-export, command=vm-trace, governance=artifact-run, workload=neural-net");
  for (auto _ : state) {
    const std::string cmd =
        shell_quote(t81_bin) + " vm trace " + shell_quote(artifact) + " -o " + shell_quote(trace_out) +
        " >/dev/null 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 vm trace neural-net failed");
      break;
    }
  }
  if (std::filesystem::exists(trace_out, ec)) {
    state.counters["trace_bytes"] = static_cast<double>(std::filesystem::file_size(trace_out, ec));
  }
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_VMTrace_Export_NeuralNet);

static void BM_GovernedCLI_AxionLog_JSON(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-axion-log");
  const auto json_out = workdir / "axion-log.json";
  std::error_code ec;
  std::filesystem::create_directories(workdir, ec);
  state.SetLabel("workflow=cli-export, command=axion-log, format=json");
  for (auto _ : state) {
    const std::string cmd =
        shell_quote(t81_bin) + " axion log --json >" + shell_quote(json_out) + " 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 axion log --json failed");
      break;
    }
  }
  if (std::filesystem::exists(json_out, ec)) {
    state.counters["json_bytes"] = static_cast<double>(std::filesystem::file_size(json_out, ec));
  }
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_AxionLog_JSON);

static void BM_GovernedCLI_CodeRun_WeightsModelHash(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-code-run-weights-model");
  const auto canonfs_root = workdir / "canonfs";
  const auto model_path = workdir / "small_model.t81w";
  const auto hash_out = workdir / "model_hash.txt";
  const auto program = repo_root / "tests" / "fixtures" / "t81lang_std_tensor" / "03_matmul_weights.t81";
  std::error_code ec;
  std::filesystem::create_directories(canonfs_root, ec);
  const std::string model_hash = prepare_cli_weights_model_hash(t81_bin, canonfs_root, model_path, hash_out);
  if (model_hash.empty()) {
    state.SkipWithError("failed to prepare CanonFS-backed weights model fixture");
    std::filesystem::remove_all(workdir, ec);
    return;
  }

  state.SetLabel(
      "workflow=cli-run, command=code-run, source=canonfs-weights-model, workload=matmul-weights");
  for (auto _ : state) {
    const std::string cmd = "env T81_CANONFS_ROOT=" + shell_quote(canonfs_root) + " " +
                            shell_quote(t81_bin) + " code run " + shell_quote(program) +
                            " --weights-model " + shell_quote_text(model_hash) +
                            " >/dev/null 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 code run --weights-model failed");
      break;
    }
  }
  if (std::filesystem::exists(model_path, ec)) {
    state.counters["model_bytes"] = static_cast<double>(std::filesystem::file_size(model_path, ec));
  }
  state.counters["model_hash_chars"] = static_cast<double>(model_hash.size());
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_CodeRun_WeightsModelHash);

static void BM_GovernedCLI_CodeRun_WeightsModelHash_WithPolicy(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-code-run-weights-model-policy");
  const auto canonfs_root = workdir / "canonfs";
  const auto model_path = workdir / "small_model.t81w";
  const auto hash_out = workdir / "model_hash.txt";
  const auto policy_path = workdir / "allow-model.apl";
  const auto program = repo_root / "tests" / "fixtures" / "t81lang_std_tensor" / "03_matmul_weights.t81";
  std::error_code ec;
  std::filesystem::create_directories(canonfs_root, ec);
  const std::string model_hash = prepare_cli_weights_model_hash(t81_bin, canonfs_root, model_path, hash_out);
  std::string model_checksum;
  if (model_hash.empty()) {
    state.SkipWithError("failed to prepare CanonFS-backed weights model fixture");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  try {
    model_checksum = t81::weights::load_t81w(model_path).checksum;
  } catch (...) {
    state.SkipWithError("failed to load CLI weights checksum");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  if (model_checksum.empty() || !write_cli_weights_policy(policy_path, model_checksum)) {
    state.SkipWithError("failed to write CLI weights policy fixture");
    std::filesystem::remove_all(workdir, ec);
    return;
  }

  state.SetLabel(
      "workflow=cli-run, command=code-run, source=canonfs-weights-model, governance=allow-policy, workload=matmul-weights");
  for (auto _ : state) {
    const std::string cmd = "env T81_CANONFS_ROOT=" + shell_quote(canonfs_root) + " " +
                            shell_quote(t81_bin) + " code run " + shell_quote(program) +
                            " --weights-model " + shell_quote_text(model_hash) +
                            " --policy " + shell_quote(policy_path) +
                            " >/dev/null 2>/dev/null";
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc != 0) {
      state.SkipWithError("t81 code run --weights-model --policy failed");
      break;
    }
  }
  if (std::filesystem::exists(model_path, ec)) {
    state.counters["model_bytes"] = static_cast<double>(std::filesystem::file_size(model_path, ec));
  }
  state.counters["model_hash_chars"] = static_cast<double>(model_hash.size());
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_CodeRun_WeightsModelHash_WithPolicy);

static void BM_GovernedCLI_CodeRun_WeightsModelHash_DenyPolicy(benchmark::State& state) {
  const auto repo_root = std::filesystem::current_path();
  const auto t81_bin = repo_root / "build" / "t81";
  const auto workdir = governed_emit_root("cli-code-run-weights-model-deny-policy");
  const auto canonfs_root = workdir / "canonfs";
  const auto model_path = workdir / "small_model.t81w";
  const auto denied_model_path = workdir / "other_model.t81w";
  const auto hash_out = workdir / "model_hash.txt";
  const auto denied_hash_out = workdir / "other_model_hash.txt";
  const auto policy_path = workdir / "deny-model.apl";
  const auto stderr_path = workdir / "deny.stderr";
  const auto program = repo_root / "tests" / "fixtures" / "t81lang_std_tensor" / "03_matmul_weights.t81";
  std::error_code ec;
  std::filesystem::create_directories(canonfs_root, ec);
  const std::string model_hash =
      prepare_cli_weights_model_hash(t81_bin, canonfs_root, model_path, hash_out, false);
  const std::string denied_hash =
      prepare_cli_weights_model_hash(t81_bin, canonfs_root, denied_model_path, denied_hash_out, true);
  std::string model_checksum;
  std::string denied_model_checksum;
  if (model_hash.empty() || denied_hash.empty() || model_hash == denied_hash) {
    state.SkipWithError("failed to prepare mismatched CanonFS-backed weights fixtures");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  try {
    model_checksum = t81::weights::load_t81w(model_path).checksum;
    denied_model_checksum = t81::weights::load_t81w(denied_model_path).checksum;
  } catch (...) {
    state.SkipWithError("failed to load CLI deny-policy checksums");
    std::filesystem::remove_all(workdir, ec);
    return;
  }
  if (model_checksum.empty() || denied_model_checksum.empty() || model_checksum == denied_model_checksum ||
      !write_cli_weights_policy(policy_path, denied_model_checksum)) {
    state.SkipWithError("failed to write CLI deny-policy fixture");
    std::filesystem::remove_all(workdir, ec);
    return;
  }

  state.SetLabel(
      "workflow=cli-run, command=code-run, source=canonfs-weights-model, governance=deny-policy, workload=matmul-weights");
  for (auto _ : state) {
    const std::string cmd = "env T81_CANONFS_ROOT=" + shell_quote(canonfs_root) + " " +
                            shell_quote(t81_bin) + " code run " + shell_quote(program) +
                            " --weights-model " + shell_quote_text(model_hash) +
                            " --policy " + shell_quote(policy_path) + " >/dev/null 2>" +
                            shell_quote(stderr_path);
    const int rc = run_shell_command(cmd);
    benchmark::DoNotOptimize(static_cast<long long>(rc));
    if (rc == 0) {
      state.SkipWithError("t81 code run deny-policy unexpectedly succeeded");
      break;
    }
  }
  if (std::filesystem::exists(stderr_path, ec)) {
    state.counters["stderr_bytes"] = static_cast<double>(std::filesystem::file_size(stderr_path, ec));
  }
  if (std::filesystem::exists(model_path, ec)) {
    state.counters["model_bytes"] = static_cast<double>(std::filesystem::file_size(model_path, ec));
  }
  state.counters["model_hash_chars"] = static_cast<double>(model_hash.size());
  std::filesystem::remove_all(workdir, ec);
}
BENCHMARK(BM_GovernedCLI_CodeRun_WeightsModelHash_DenyPolicy);

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
