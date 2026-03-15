// tests/determinism/evidence_collector.cpp
//
// RFC-0032 Phase 5 (C-06): promoted determinism evidence collector.
// Promoted from: experiments/ai/determinism/evidence_collector.cpp
//
// Violations removed:
//   • std::chrono timing fields in ExecutionEvidence (wall-clock, non-deterministic)
//   • std::chrono::system_clock::now() for timestamps (non-deterministic)
//   • nlohmann/json output (external dependency → plain evidence-schema-v1 text)
//   • openssl/sha.h (external dependency → FNV-1a, integer-only, no deps)
//
// Evidence schema (§ evidence-schema-v1):
//   Plain key=value text — see tests/determinism/README.md
//
// Gate tests [C06-01..06]:
//   [C06-01]  hex_hash is stable: same input → same hash across two calls.
//   [C06-02]  hex_hash distinguishes different inputs.
//   [C06-03]  validate_determinism() returns true for 3 identical runs.
//   [C06-04]  validate_determinism() returns false when outputs differ.
//   [C06-05]  write_schema() produces well-formed evidence-schema-v1 output.
//   [C06-06]  record_trace() accepts AIHookEngine::ai_trace(); hash is
//             reproducible across two independent AIHookEngine instances given
//             the same input SyscallContext.

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/ai_model_loader.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// EvidenceCollector — integer-only, no external deps, no wall-clock timing.
// ─────────────────────────────────────────────────────────────────────────────

namespace t81::determinism {

// FNV-1a 64-bit hash — deterministic, integer-only, no external deps.
// Reference: http://www.isthe.com/chongo/tech/comp/fnv/
static constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
static constexpr std::uint64_t kFnvPrime  = 1099511628211ULL;

[[nodiscard]] static std::uint64_t fnv1a(const std::string& data) noexcept {
  std::uint64_t h = kFnvOffset;
  for (unsigned char c : data) {
    h ^= static_cast<std::uint64_t>(c);
    h *= kFnvPrime;
  }
  return h;
}

[[nodiscard]] static std::string hex_hash(const std::string& data) {
  char buf[17];
  std::snprintf(buf, sizeof(buf), "%016llx",
                static_cast<unsigned long long>(fnv1a(data)));
  return std::string(buf);
}

// Single-run evidence record.
// No timing fields — see tests/determinism/README.md §evidence-schema-v1.
struct ExecutionEvidence {
  std::string input_hash;   // FNV-1a hex of raw input data
  std::string output_hash;  // FNV-1a hex of raw output data
  std::string trace_hash;   // FNV-1a hex of concatenated trace events
  std::map<std::string, std::string> metrics;
};

// EvidenceCollector collects per-run determinism evidence and validates that
// all recorded runs produce identical hashes (strict determinism).
class EvidenceCollector {
 public:
  explicit EvidenceCollector(std::string experiment_name)
      : experiment_name_(std::move(experiment_name)) {}

  // Begin a new run.
  void start_collection(const std::string& input_data) {
    ExecutionEvidence ev;
    ev.input_hash = hex_hash(input_data);
    executions_.push_back(std::move(ev));
  }

  // Record the output hash for the current run.
  void record_output(const std::string& output_data) {
    if (!executions_.empty())
      executions_.back().output_hash = hex_hash(output_data);
  }

  // Record a trace vector.  Compatible with AIHookEngine::ai_trace().
  // All events are joined with '\n' before hashing.
  void record_trace(const std::vector<std::string>& trace_events) {
    if (executions_.empty()) return;
    std::string flat;
    flat.reserve(trace_events.size() * 64);
    for (const auto& e : trace_events) { flat += e; flat += '\n'; }
    executions_.back().trace_hash = hex_hash(flat);
  }

  // Record arbitrary key=value metrics for the current run.
  void record_metrics(const std::map<std::string, std::string>& m) {
    if (!executions_.empty())
      executions_.back().metrics = m;
  }

  // Returns true iff ≥2 runs all have identical input/output/trace hashes.
  [[nodiscard]] bool validate_determinism() const noexcept {
    if (executions_.size() < 2) return false;
    const auto& ref = executions_[0];
    for (std::size_t i = 1; i < executions_.size(); ++i) {
      const auto& cur = executions_[i];
      if (cur.input_hash  != ref.input_hash)  return false;
      if (cur.output_hash != ref.output_hash) return false;
      if (cur.trace_hash  != ref.trace_hash)  return false;
    }
    return true;
  }

  // Write evidence in evidence-schema-v1 format (key=value, one per line).
  // See tests/determinism/README.md for schema specification.
  void write_schema(const std::filesystem::path& out_path) const {
    std::filesystem::create_directories(out_path.parent_path());
    std::ofstream f(out_path);
    f << "schema_version=evidence-schema-v1\n";
    f << "experiment=" << experiment_name_ << "\n";
    f << "run_count=" << executions_.size() << "\n";
    for (std::size_t i = 0; i < executions_.size(); ++i) {
      const auto& ev = executions_[i];
      f << "run." << i << ".input_hash="  << ev.input_hash  << "\n";
      f << "run." << i << ".output_hash=" << ev.output_hash << "\n";
      f << "run." << i << ".trace_hash="  << ev.trace_hash  << "\n";
      for (const auto& [k, v] : ev.metrics)
        f << "run." << i << ".metric." << k << "=" << v << "\n";
    }
    f << "determinism_pass=" << (validate_determinism() ? "true" : "false") << "\n";
  }

  [[nodiscard]] std::size_t run_count() const noexcept { return executions_.size(); }

 private:
  std::string experiment_name_;
  std::vector<ExecutionEvidence> executions_;
};

}  // namespace t81::determinism

// ─────────────────────────────────────────────────────────────────────────────
// Gate tests [C06-01..06]
// ─────────────────────────────────────────────────────────────────────────────

static int g_pass = 0, g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
}

static bool contains(const std::string& s, const std::string& sub) {
  return s.find(sub) != std::string::npos;
}

// [C06-01] Same input → same hash across two independent calls.
static void test_hash_stability() {
  std::printf("\n[C06-01] hex_hash stable across repeated calls\n");
  const std::string data = "sha3-256:aabbccdd11223344";
  check(t81::determinism::hex_hash(data) == t81::determinism::hex_hash(data),
        "[C06-01] same input → same hash");
}

// [C06-02] Different inputs → different hashes.
static void test_hash_collision() {
  std::printf("\n[C06-02] hex_hash distinguishes different inputs\n");
  check(t81::determinism::hex_hash("abc") != t81::determinism::hex_hash("def"),
        "[C06-02] 'abc' and 'def' have distinct hashes");
  check(t81::determinism::hex_hash("") != t81::determinism::hex_hash("x"),
        "[C06-02] empty and 'x' have distinct hashes");
}

// [C06-03] validate_determinism() true for identical runs.
static void test_validate_pass() {
  std::printf("\n[C06-03] validate_determinism() passes for 3 identical runs\n");
  t81::determinism::EvidenceCollector col("test-pass");
  for (int i = 0; i < 3; ++i) {
    col.start_collection("input_data");
    col.record_output("output_data");
    col.record_trace({"model_load success hash=sha3-256:aabb reason=allow",
                      "ai_exec_gate backend=t81vm policy=allow"});
  }
  check(col.validate_determinism(), "[C06-03] 3 identical runs → determinism pass");
  check(col.run_count() == 3,       "[C06-03] run_count==3");
}

// [C06-04] validate_determinism() false when output differs between runs.
static void test_validate_fail() {
  std::printf("\n[C06-04] validate_determinism() fails when outputs differ\n");
  t81::determinism::EvidenceCollector col("test-fail");
  col.start_collection("input");
  col.record_output("output_A");
  col.record_trace({"event_X"});
  col.start_collection("input");
  col.record_output("output_B");  // different output
  col.record_trace({"event_X"});
  check(!col.validate_determinism(),
        "[C06-04] differing outputs → determinism fail");
}

// [C06-05] write_schema() produces well-formed evidence-schema-v1 output.
static void test_write_schema() {
  std::printf("\n[C06-05] write_schema() produces valid evidence-schema-v1\n");

  namespace fs = std::filesystem;
  const fs::path out_dir = "/tmp/t81_phase5_c06";
  const fs::path out     = out_dir / "evidence.txt";
  fs::remove_all(out_dir);

  t81::determinism::EvidenceCollector col("schema-gate");
  for (int i = 0; i < 2; ++i) {
    col.start_collection("abc");
    col.record_output("xyz");
    col.record_trace({"trace_event_alpha"});
    col.record_metrics({{"trit_ops", "81"}, {"tensors", "3"}});
  }
  col.write_schema(out);

  std::ifstream f(out);
  std::string content((std::istreambuf_iterator<char>(f)), {});

  check(contains(content, "schema_version=evidence-schema-v1"),
        "[C06-05] schema_version line present");
  check(contains(content, "experiment=schema-gate"),
        "[C06-05] experiment name present");
  check(contains(content, "run_count=2"),
        "[C06-05] run_count=2 present");
  check(contains(content, "run.0.input_hash="),
        "[C06-05] run.0.input_hash line present");
  check(contains(content, "run.1.output_hash="),
        "[C06-05] run.1.output_hash line present");
  check(contains(content, "determinism_pass=true"),
        "[C06-05] determinism_pass=true when runs are identical");
  check(contains(content, "run.0.metric.trit_ops=81"),
        "[C06-05] metric line written correctly");

  fs::remove_all(out_dir);
}

// [C06-06] record_trace() from real AIHookEngine; hash is reproducible.
static void test_trace_from_ai_hooks() {
  std::printf("\n[C06-06] record_trace() from AIHookEngine::ai_trace() is reproducible\n");

  // Run the same QMATMUL evaluation twice through independent hook instances.
  t81::axion::SyscallContext ctx;
  ctx.next_opcode  = t81::tisc::Opcode::QMATMUL;
  ctx.current_tier = 2;
  ctx.payload      = "scale=8 wt_hash=sha3-256:cafecafe00112233";

  auto make_trace = [&]() {
    auto inner = std::make_unique<t81::axion::PolicyEngine>(t81::axion::Policy{});
    auto hook  = std::make_unique<t81::axion::AIHookEngine>(std::move(inner));
    (void)hook->evaluate(ctx);
    return hook->ai_trace();
  };

  const auto trace1 = make_trace();
  const auto trace2 = make_trace();

  check(!trace1.empty(),
        "[C06-06] AIHookEngine trace non-empty after QMATMUL evaluate");

  t81::determinism::EvidenceCollector col("hook-trace-determinism");
  col.start_collection("qmatmul_ctx");
  col.record_trace(trace1);
  col.start_collection("qmatmul_ctx");
  col.record_trace(trace2);

  check(col.validate_determinism(),
        "[C06-06] same QMATMUL input → identical trace hash across two hook instances");
}

int main() {
  std::printf("=== Evidence collector tests (RFC-0032 Phase 5 C-06) ===\n");

  test_hash_stability();
  test_hash_collision();
  test_validate_pass();
  test_validate_fail();
  test_write_schema();
  test_trace_from_ai_hooks();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
