#include "test_runtime_check.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

struct CommandResult {
  std::string stdout_text;
  std::string stderr_text;
  int exit_code = 0;
};

std::string shell_escape(std::string_view arg) {
  if (arg.empty()) {
    return "''";
  }
  bool needs_quote = false;
  for (char c : arg) {
    if (std::isspace(static_cast<unsigned char>(c)) || c == '"' || c == '\'' || c == '\\' ||
        c == '$' || c == '&' || c == '|' || c == ';' || c == '<' || c == '>' || c == '*' ||
        c == '?' || c == '~' || c == '`' || c == '(' || c == ')' || c == '[' || c == ']' ||
        c == '{' || c == '}') {
      needs_quote = true;
      break;
    }
  }
  if (!needs_quote) {
    return std::string(arg);
  }
  std::string escaped = "'";
  for (char c : arg) {
    if (c == '\'') {
      escaped += "'\\''";
    } else {
      escaped.push_back(c);
    }
  }
  escaped.push_back('\'');
  return escaped;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

CommandResult run_cli(const fs::path& bin_path, const std::vector<std::string>& args) {
  const fs::path temp_root = fs::temp_directory_path();
  const fs::path out_path = temp_root / "t81-cli-contract.out";
  const fs::path err_path = temp_root / "t81-cli-contract.err";
  const fs::path code_path = temp_root / "t81-cli-contract.code";

  std::string cmd = shell_escape(fs::absolute(bin_path).string());
  for (const auto& arg : args) {
    cmd += " ";
    cmd += shell_escape(arg);
  }
  cmd += " > ";
  cmd += shell_escape(out_path.string());
  cmd += " 2> ";
  cmd += shell_escape(err_path.string());
  cmd += "; echo $? > ";
  cmd += shell_escape(code_path.string());

  const int rc = std::system(cmd.c_str());
  T81_TEST_CHECK(rc == 0);

  CommandResult result;
  result.stdout_text = read_file(out_path);
  result.stderr_text = read_file(err_path);

  std::ifstream code_in(code_path);
  T81_TEST_CHECK(static_cast<bool>(code_in));
  code_in >> result.exit_code;
  T81_TEST_CHECK(static_cast<bool>(code_in));

  return result;
}

bool contains(std::string_view text, std::string_view pattern) {
  return text.find(pattern) != std::string_view::npos;
}

fs::path make_temp_path(const std::string& prefix, const std::string& extension) {
  static std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<uint64_t> dist;
  return fs::temp_directory_path() / (prefix + "-" + std::to_string(dist(rng)) + extension);
}

int main(int argc, char* argv[]) {
  T81_TEST_CHECK(argc >= 2);
  const fs::path t81_bin = fs::path(argv[1]);
  T81_TEST_CHECK(fs::exists(t81_bin));

  // Test basic CLI functionality - minimal set to avoid hanging
  {
    const auto result = run_cli(t81_bin, {"help", "compile"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 compile"));
  }

  {
    const fs::path invalid_file = make_temp_path("t81-cli-contract-invalid", ".t81");
    {
      std::ofstream out(invalid_file);
      out << "fn main( {\n";
    }
    const auto result = run_cli(t81_bin, {"check", invalid_file.string()});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, ":1:"));
    T81_TEST_CHECK(contains(result.stderr_text, "^"));
    std::error_code ignore_ec;
    fs::remove(invalid_file, ignore_ec);
  }

  {
    const auto result = run_cli(t81_bin, {"--help"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "code    <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "canonfs <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "determinism <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "vm <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "tisc <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "ir <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "weights <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "policy <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "axion <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "trace <action> [args]"));
    T81_TEST_CHECK(!contains(result.stderr_text, "compile <file.t81|.t81w>"));
    T81_TEST_CHECK(!contains(result.stderr_text, "check   <file.t81>"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "advanced"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "verify/quantize"));
    T81_TEST_CHECK(contains(result.stderr_text, "simulate/explain/snapshot/snapshot-diff/rollback"));
    T81_TEST_CHECK(contains(result.stderr_text, "encode/decode"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "labs"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Runtime-backed memory pool profiling"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "canonfs"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 canonfs <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "determinism"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 determinism <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "vm"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 vm <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "vm", "trace"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 vm trace"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "determinism", "verify-run"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 determinism verify-run"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "determinism", "explain"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 determinism explain"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "axion", "snapshot-diff"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 axion snapshot-diff"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "trace", "summary"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 trace summary"));
  }

  {
    const auto result = run_cli(t81_bin, {"env", "paths", "--json"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.env-paths.v1\""));
  }

  {
    const auto result = run_cli(t81_bin, {"env", "toolchain", "--json"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.env-toolchain.v1\""));
  }

  {
    const auto result = run_cli(t81_bin, {"env", "diag", "--json"});
    T81_TEST_CHECK(result.exit_code == 0 || result.exit_code == 2);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.env-diag.v1\""));
    T81_TEST_CHECK(contains(result.stdout_text, "\"repo_root\":"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "tisc"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 tisc <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "ir"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 ir <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"axion", "optimize", "--tier", "nope", "--json"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "--tier must be an integer"));
  }

  {
    const fs::path t81_file = make_temp_path("t81-cli-contract", ".t81");
    const fs::path tisc_file = make_temp_path("t81-cli-contract", ".tisc");
    const fs::path base81_out = make_temp_path("t81-cli-contract", ".base81");
    const fs::path trace_file = make_temp_path("t81-cli-contract", ".trace");
    const fs::path trace_extra = make_temp_path("t81-cli-contract", ".trace-extra");
    const fs::path policy_file = make_temp_path("t81-cli-contract", ".apl");

    {
      std::ofstream out(t81_file);
      out << "fn main() -> i32 {\n"
             "  print(\"Hello, Trace!\");\n"
             "  return 0;\n"
             "}\n";
    }

    const auto build_result =
        run_cli(t81_bin, {"code", "build", t81_file.string(), "-o", tisc_file.string()});
    T81_TEST_CHECK(build_result.exit_code == 0);

    {
      std::ofstream out(policy_file);
      out << "(policy (tier 1) (allowed-tensor-hashes [\"sha3-256:test-hash\"]))\n";
    }

    const auto run_result = run_cli(
        t81_bin, {"code", "run", tisc_file.string(), "--trace-out", trace_file.string()});
    T81_TEST_CHECK(run_result.exit_code == 0);
    T81_TEST_CHECK(contains(run_result.stdout_text, "Hello, Trace!"));
    T81_TEST_CHECK(!contains(run_result.stdout_text, "PC="));

    const std::string trace_text = read_file(trace_file);
    T81_TEST_CHECK(contains(trace_text, "PC="));

    const auto replay_result =
        run_cli(t81_bin, {"trace", "replay", tisc_file.string(), trace_file.string(), "--json"});
    T81_TEST_CHECK(replay_result.exit_code == 0);
    T81_TEST_CHECK(contains(replay_result.stdout_text, "\"ok\": true"));

    const fs::path missing_trace = make_temp_path("t81-cli-contract-missing", ".trace");
    const auto replay_missing_result =
        run_cli(t81_bin, {"trace", "replay", tisc_file.string(), missing_trace.string(), "--json"});
    T81_TEST_CHECK(replay_missing_result.exit_code != 0);
    T81_TEST_CHECK(contains(replay_missing_result.stdout_text, "\"kind\": \"open_error\""));
    T81_TEST_CHECK(replay_missing_result.stderr_text.empty());

    {
      std::ofstream out(trace_extra);
      out << trace_text << "PC=999 Halt\n";
    }
    const auto diff_result =
        run_cli(t81_bin, {"trace", "diff", trace_file.string(), trace_extra.string(), "--no-color"});
    T81_TEST_CHECK(diff_result.exit_code != 0);
    T81_TEST_CHECK(contains(diff_result.stdout_text, "Difference at line"));

    const auto trace_hash_result =
        run_cli(t81_bin, {"determinism", "trace-hash", trace_file.string(), "--json"});
    T81_TEST_CHECK(trace_hash_result.exit_code == 0);
    T81_TEST_CHECK(contains(trace_hash_result.stdout_text, "\"schema\": \"t81.determinism-trace-hash.v1\""));

    const auto trace_summary_result =
        run_cli(t81_bin, {"trace", "summary", trace_file.string(), "--json"});
    T81_TEST_CHECK(trace_summary_result.exit_code == 0);
    T81_TEST_CHECK(contains(trace_summary_result.stdout_text, "\"schema\": \"t81.trace-summary.v1\""));

    const auto canonicalize_result =
        run_cli(t81_bin, {"trace", "canonicalize", trace_file.string(), "-o", trace_extra.string()});
    T81_TEST_CHECK(canonicalize_result.exit_code == 0);
    T81_TEST_CHECK(!read_file(trace_extra).empty());
    T81_TEST_CHECK(contains(read_file(trace_extra), "PC="));

    const auto diff_trace_result = run_cli(
        t81_bin,
        {"determinism", "diff-trace", trace_file.string(), trace_file.string(), "--json"});
    T81_TEST_CHECK(diff_trace_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(diff_trace_result.stdout_text, "\"schema\": \"t81.determinism-trace-diff.v1\""));

    const auto verify_run_result = run_cli(
        t81_bin, {"determinism", "verify-run", tisc_file.string(), "--json"});
    T81_TEST_CHECK(verify_run_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(verify_run_result.stdout_text, "\"schema\": \"t81.determinism-verify-run.v1\""));

    const auto certify_result = run_cli(
        t81_bin, {"determinism", "certify", tisc_file.string(), "--json"});
    T81_TEST_CHECK(certify_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(certify_result.stdout_text, "\"schema\": \"t81.determinism-certificate.v1\""));
    T81_TEST_CHECK(contains(certify_result.stdout_text, "\"deterministic\": true"));

    const auto explain_result = run_cli(
        t81_bin, {"determinism", "explain", tisc_file.string(), "--json"});
    T81_TEST_CHECK(explain_result.exit_code == 0);
    T81_TEST_CHECK(contains(explain_result.stdout_text, "\"schema\": \"t81.determinism-explain.v1\""));

    const auto vm_trace_result =
        run_cli(t81_bin, {"vm", "trace", tisc_file.string(), "--out", trace_extra.string()});
    T81_TEST_CHECK(vm_trace_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_trace_result.stdout_text, "Trace written to") ||
                   contains(vm_trace_result.stderr_text, "Trace written to"));
    T81_TEST_CHECK(contains(read_file(trace_extra), "PC="));

    const auto vm_trace_short_result =
        run_cli(t81_bin, {"vm", "trace", tisc_file.string(), "-o", trace_extra.string()});
    T81_TEST_CHECK(vm_trace_short_result.exit_code == 0);
    T81_TEST_CHECK(contains(read_file(trace_extra), "PC="));

    const auto vm_step_result =
        run_cli(t81_bin, {"vm", "step", tisc_file.string(), "--count", "2", "--json"});
    T81_TEST_CHECK(vm_step_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_step_result.stdout_text, "\"schema\": \"t81.vm-step.v1\""));

    const auto vm_regs_result =
        run_cli(t81_bin, {"vm", "regs", tisc_file.string(), "--steps", "2", "--json"});
    T81_TEST_CHECK(vm_regs_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_regs_result.stdout_text, "\"schema\": \"t81.vm-regs.v1\""));

    const auto vm_stack_result =
        run_cli(t81_bin, {"vm", "stack", tisc_file.string(), "--steps", "2", "--json"});
    T81_TEST_CHECK(vm_stack_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_stack_result.stdout_text, "\"schema\": \"t81.vm-stack.v1\""));

    const auto vm_mem_result =
        run_cli(t81_bin, {"vm", "mem", tisc_file.string(), "--addr", "0", "--steps", "2", "--json"});
    T81_TEST_CHECK(vm_mem_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_mem_result.stdout_text, "\"schema\": \"t81.vm-mem.v1\""));

    const auto vm_state_result =
        run_cli(t81_bin, {"vm", "state", tisc_file.string(), "--json"});
    T81_TEST_CHECK(vm_state_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_state_result.stdout_text, "\"schema\": \"t81.vm-state.v1\""));

    const auto vm_profile_result =
        run_cli(t81_bin, {"vm", "profile", tisc_file.string(), "--json"});
    T81_TEST_CHECK(vm_profile_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_profile_result.stdout_text, "\"schema\": \"t81.vm-profile.v1\""));

    const auto tisc_validate_result =
        run_cli(t81_bin, {"tisc", "validate", tisc_file.string(), "--json"});
    T81_TEST_CHECK(tisc_validate_result.exit_code == 0);
    T81_TEST_CHECK(contains(tisc_validate_result.stdout_text, "\"schema\": \"t81.tisc-validate.v1\""));

    const auto tisc_decode_result =
        run_cli(t81_bin, {"tisc", "decode", tisc_file.string(), "-o", base81_out.string(), "--json"});
    T81_TEST_CHECK(tisc_decode_result.exit_code == 0);
    T81_TEST_CHECK(contains(tisc_decode_result.stdout_text, "\"schema\": \"t81.tisc-decode.v1\""));
    T81_TEST_CHECK(!read_file(base81_out).empty());

    const auto ir_show_result = run_cli(t81_bin, {"ir", "show", t81_file.string()});
    T81_TEST_CHECK(ir_show_result.exit_code == 0);
    T81_TEST_CHECK(contains(ir_show_result.stdout_text, "IR Instructions"));
    T81_TEST_CHECK(contains(ir_show_result.stdout_text, "PRINT"));

    const auto ir_export_result = run_cli(t81_bin, {"ir", "export", t81_file.string(), "--json"});
    T81_TEST_CHECK(ir_export_result.exit_code == 0);
    T81_TEST_CHECK(contains(ir_export_result.stdout_text, "\"schema\": \"t81.ir-export.v1\""));

    const auto policy_test_result = run_cli(
        t81_bin, {"policy", "test", policy_file.string(), "--model-hash", "sha3-256:test-hash", "--json"});
    T81_TEST_CHECK(policy_test_result.exit_code == 0);
    T81_TEST_CHECK(contains(policy_test_result.stdout_text, "\"schema\": \"t81.policy-test.v1\""));

    const auto axion_explain_result =
        run_cli(t81_bin, {"axion", "explain", policy_file.string(), "--json"});
    T81_TEST_CHECK(axion_explain_result.exit_code == 0);
    T81_TEST_CHECK(contains(axion_explain_result.stdout_text, "\"schema\":\"t81.axion-explain.v1\""));

    const auto axion_status_result = run_cli(t81_bin, {"axion", "status", "--json"});
    T81_TEST_CHECK(axion_status_result.exit_code == 0);
    T81_TEST_CHECK(contains(axion_status_result.stdout_text, "\"schema\": \"t81.axion-status.v1\""));

    const auto axion_optimize_result = run_cli(t81_bin, {"axion", "optimize", "--tier", "2", "--json"});
    T81_TEST_CHECK(axion_optimize_result.exit_code == 0);
    T81_TEST_CHECK(contains(axion_optimize_result.stdout_text, "\"schema\": \"t81.axion-optimize.v1\""));

    const auto axion_snapshot_result = run_cli(t81_bin, {"axion", "snapshot", "--json"});
    T81_TEST_CHECK(axion_snapshot_result.exit_code == 0);
    const auto axion_hash_pos = axion_snapshot_result.stdout_text.find("\"hash\": \"");
    T81_TEST_CHECK(axion_hash_pos != std::string::npos);
    const auto axion_hash_start = axion_hash_pos + std::string("\"hash\": \"").size();
    const auto axion_hash_end = axion_snapshot_result.stdout_text.find('"', axion_hash_start);
    T81_TEST_CHECK(axion_hash_end != std::string::npos);
    const std::string axion_snapshot_hash =
        axion_snapshot_result.stdout_text.substr(axion_hash_start, axion_hash_end - axion_hash_start);

    const auto axion_snapshot_diff_result =
        run_cli(t81_bin, {"axion", "snapshot-diff", axion_snapshot_hash, axion_snapshot_hash, "--json"});
    T81_TEST_CHECK(axion_snapshot_diff_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(axion_snapshot_diff_result.stdout_text, "\"schema\": \"t81.canonfs-snapshot-diff.v1\""));

    const auto weights_verify_help = run_cli(t81_bin, {"help", "weights", "verify"});
    T81_TEST_CHECK(weights_verify_help.exit_code == 0);
    T81_TEST_CHECK(contains(weights_verify_help.stderr_text, "Usage: t81 weights verify"));

    const auto lang_help = run_cli(t81_bin, {"help", "lang"});
    T81_TEST_CHECK(lang_help.exit_code == 0);
    T81_TEST_CHECK(contains(lang_help.stderr_text, "Usage: t81 lang <action>"));

    const auto lang_export_result = run_cli(t81_bin, {"lang", "export", t81_file.string(), "--json"});
    T81_TEST_CHECK(lang_export_result.exit_code == 0);
    T81_TEST_CHECK(contains(lang_export_result.stdout_text, "\"schema\": \"t81.ir-export.v1\""));

    const auto model_help = run_cli(t81_bin, {"help", "model"});
    T81_TEST_CHECK(model_help.exit_code == 0);
    T81_TEST_CHECK(contains(model_help.stderr_text, "Compatibility alias for `t81 weights`"));

    const auto tensor_help = run_cli(t81_bin, {"help", "tensor", "hash"});
    T81_TEST_CHECK(tensor_help.exit_code == 0);
    T81_TEST_CHECK(contains(tensor_help.stderr_text, "Usage: t81 tensor hash"));

    const auto bench_help = run_cli(t81_bin, {"help", "bench"});
    T81_TEST_CHECK(bench_help.exit_code == 0);
    T81_TEST_CHECK(contains(bench_help.stderr_text, "Compatibility alias for `t81 benchmark`"));

    std::error_code ignore_ec;
    fs::remove(t81_file, ignore_ec);
    fs::remove(tisc_file, ignore_ec);
    fs::remove(base81_out, ignore_ec);
    fs::remove(trace_file, ignore_ec);
    fs::remove(trace_extra, ignore_ec);
    fs::remove(policy_file, ignore_ec);
  }

  {
    const fs::path payload = make_temp_path("t81-cli-contract", ".txt");
    {
      std::ofstream out(payload);
      out << "canonfs-contract";
    }
    const auto put_result = run_cli(t81_bin, {"canonfs", "put-file", payload.string()});
    T81_TEST_CHECK(put_result.exit_code == 0);
    T81_TEST_CHECK(contains(put_result.stdout_text, "sha3-256:"));
    std::string hash = put_result.stdout_text;
    while (!hash.empty() && std::isspace(static_cast<unsigned char>(hash.back()))) hash.pop_back();

    const auto stat_result = run_cli(t81_bin, {"canonfs", "stat", hash, "--json"});
    T81_TEST_CHECK(stat_result.exit_code == 0);
    T81_TEST_CHECK(contains(stat_result.stdout_text, "\"schema\": \"t81.canonfs-stat.v1\""));
    T81_TEST_CHECK(contains(stat_result.stdout_text, "\"ok\": true"));

    const auto list_result = run_cli(t81_bin, {"canonfs", "ls", "--json"});
    T81_TEST_CHECK(list_result.exit_code == 0);
    T81_TEST_CHECK(contains(list_result.stdout_text, "\"schema\": \"t81.canonfs-list.v1\""));
    T81_TEST_CHECK(contains(list_result.stdout_text, hash));

    const auto verify_result = run_cli(t81_bin, {"canonfs", "verify", hash, "--json"});
    T81_TEST_CHECK(verify_result.exit_code == 0);
    T81_TEST_CHECK(contains(verify_result.stdout_text, "\"schema\": \"t81.canonfs-verify.v1\""));

    const fs::path restored = make_temp_path("t81-cli-contract", ".restored");
    const auto get_result =
        run_cli(t81_bin, {"canonfs", "get", hash, "--out", restored.string(), "--json"});
    T81_TEST_CHECK(get_result.exit_code == 0);
    T81_TEST_CHECK(contains(get_result.stdout_text, "\"schema\": \"t81.canonfs-get.v1\""));
    T81_TEST_CHECK(read_file(restored) == "canonfs-contract");

    const auto snapshot_a = run_cli(t81_bin, {"canonfs", "snapshot", "--json"});
    T81_TEST_CHECK(snapshot_a.exit_code == 0);
    const auto first_hash_pos = snapshot_a.stdout_text.find("\"hash\": \"");
    T81_TEST_CHECK(first_hash_pos != std::string::npos);
    const auto first_hash_start = first_hash_pos + std::string("\"hash\": \"").size();
    const auto first_hash_end = snapshot_a.stdout_text.find('"', first_hash_start);
    T81_TEST_CHECK(first_hash_end != std::string::npos);
    const std::string snapshot_hash_a =
        snapshot_a.stdout_text.substr(first_hash_start, first_hash_end - first_hash_start);

    const fs::path payload_two = make_temp_path("t81-cli-contract", ".txt2");
    {
      std::ofstream out(payload_two);
      out << "canonfs-contract-two-" << payload_two.filename().string();
    }
    const auto put_result_two = run_cli(t81_bin, {"canonfs", "put-file", payload_two.string()});
    T81_TEST_CHECK(put_result_two.exit_code == 0);

    const auto snapshot_b = run_cli(t81_bin, {"canonfs", "snapshot", "--json"});
    T81_TEST_CHECK(snapshot_b.exit_code == 0);
    const auto second_hash_pos = snapshot_b.stdout_text.find("\"hash\": \"");
    T81_TEST_CHECK(second_hash_pos != std::string::npos);
    const auto second_hash_start = second_hash_pos + std::string("\"hash\": \"").size();
    const auto second_hash_end = snapshot_b.stdout_text.find('"', second_hash_start);
    T81_TEST_CHECK(second_hash_end != std::string::npos);
    const std::string snapshot_hash_b =
        snapshot_b.stdout_text.substr(second_hash_start, second_hash_end - second_hash_start);

    const auto snapshot_diff_result = run_cli(
        t81_bin, {"canonfs", "snapshot-diff", snapshot_hash_a, snapshot_hash_b, "--json"});
    T81_TEST_CHECK(snapshot_diff_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(snapshot_diff_result.stdout_text, "\"schema\": \"t81.canonfs-snapshot-diff.v1\""));

    std::error_code ignore_ec;
    fs::remove(payload, ignore_ec);
    fs::remove(restored, ignore_ec);
    fs::remove(payload_two, ignore_ec);
  }

  {
    const fs::path hello_world = fs::absolute(t81_bin).parent_path().parent_path() / "examples" / "hello_world.t81";
    const auto memory_stats_result = run_cli(t81_bin, {"memory-stats", hello_world.string()});
    T81_TEST_CHECK(memory_stats_result.exit_code == 0);
    T81_TEST_CHECK(contains(memory_stats_result.stdout_text, "Memory Pool Analysis"));
    T81_TEST_CHECK(contains(memory_stats_result.stdout_text, "Stack peak usage"));
    T81_TEST_CHECK(!contains(memory_stats_result.stderr_text, "legacy alias"));
  }

  {
    const auto bare_vm_result = run_cli(t81_bin, {"vm"});
    T81_TEST_CHECK(bare_vm_result.exit_code == 0);
    T81_TEST_CHECK(contains(bare_vm_result.stderr_text, "Usage: t81 vm <action> [args]"));

    const auto bare_trace_result = run_cli(t81_bin, {"trace"});
    T81_TEST_CHECK(bare_trace_result.exit_code == 0);
    T81_TEST_CHECK(contains(bare_trace_result.stderr_text, "Usage: t81 trace <subcommand> [args]"));

    const auto completion_result = run_cli(t81_bin, {"completion", "fish"});
    T81_TEST_CHECK(completion_result.exit_code == 0);
    T81_TEST_CHECK(!contains(completion_result.stdout_text, " compile "));
    T81_TEST_CHECK(!contains(completion_result.stdout_text, " check "));
  }

  {
    const fs::path invalid_base81 = make_temp_path("t81-cli-contract-invalid", ".base81");
    {
      std::ofstream out(invalid_base81);
      out << "not-valid-base81";
    }
    const auto result = run_cli(t81_bin, {"tisc", "encode", invalid_base81.string()});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "failed to parse Base81 view:"));
    std::error_code ignore_ec;
    fs::remove(invalid_base81, ignore_ec);
  }

  // Test pkg check functionality - the critical part that was failing
  {
    const fs::path package_file = "package.t81";
    const fs::path bad_file = "bad.t81";
    
    // Clean up any existing files
    std::error_code ignore_ec;
    fs::remove(package_file, ignore_ec);
    fs::remove(bad_file, ignore_ec);

    {
      std::ofstream out(package_file);
      out << "(package\n"
             "  (name \"ok_pkg\")\n"
             "  (version \"1.2.3\")\n"
             ")\n";
    }
    const auto ok_result = run_cli(t81_bin, {"pkg", "check", package_file.string(), "--json"});
    T81_TEST_CHECK(ok_result.exit_code == 0);
    T81_TEST_CHECK(contains(ok_result.stdout_text, "\"schema\": \"t81.pkg-check.v1\""));
    T81_TEST_CHECK(contains(ok_result.stdout_text, "\"valid\": true"));

    {
      std::ofstream out(bad_file);
      out << "(package\n"
             "  (name \"bad pkg\")\n"
             "  (version \"x.y.z\")\n"
             ")\n";
    }
    const auto bad_result = run_cli(t81_bin, {"pkg", "check", bad_file.string(), "--json"});
    T81_TEST_CHECK(bad_result.exit_code == 2);
    T81_TEST_CHECK(contains(bad_result.stdout_text, "\"valid\": false"));

    // Clean up
    fs::remove(package_file, ignore_ec);
    fs::remove(bad_file, ignore_ec);
  }

  std::cout << "CLI contract tests PASSED!\n";
  return 0;
}
