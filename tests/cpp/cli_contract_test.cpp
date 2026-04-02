#include "t81/weights.hpp"
#include "test_runtime_check.hpp"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

void set_env(const char* name, const std::string& value) {
#if defined(_WIN32)
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

void unset_env(const char* name) {
#if defined(_WIN32)
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

}  // namespace

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

fs::path make_temp_path(const std::string& prefix, const std::string& extension) {
  static std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<uint64_t> dist;
  return fs::temp_directory_path() / (prefix + "-" + std::to_string(dist(rng)) + extension);
}

CommandResult run_cli_in_dir(const fs::path& bin_path, const std::vector<std::string>& args,
                             const std::optional<fs::path>& workdir,
                             std::chrono::seconds timeout = std::chrono::seconds(20));

CommandResult run_cli(const fs::path& bin_path, const std::vector<std::string>& args) {
  return run_cli_in_dir(bin_path, args, std::nullopt);
}

CommandResult run_cli_with_timeout(const fs::path& bin_path, const std::vector<std::string>& args,
                                   std::chrono::seconds timeout) {
  return run_cli_in_dir(bin_path, args, std::nullopt, timeout);
}

CommandResult run_cli_in_dir(const fs::path& bin_path, const std::vector<std::string>& args,
                             const std::optional<fs::path>& workdir, std::chrono::seconds timeout) {
  const fs::path out_path = make_temp_path("t81-cli-contract", ".out");
  const fs::path err_path = make_temp_path("t81-cli-contract", ".err");
  std::vector<std::string> argv_storage;
  argv_storage.push_back(fs::absolute(bin_path).string());
  argv_storage.insert(argv_storage.end(), args.begin(), args.end());

  std::ostringstream step;
  for (std::size_t i = 0; i < argv_storage.size(); ++i) {
    if (i != 0) {
      step << " ";
    }
    step << argv_storage[i];
  }
  std::cerr << "[cli] " << step.str() << "\n";

  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(argv_storage.size() + 1);
  for (auto& item : argv_storage) {
    argv_ptrs.push_back(item.data());
  }
  argv_ptrs.push_back(nullptr);

  pid_t pid = fork();
  T81_TEST_CHECK(pid >= 0);
  if (pid == 0) {
    if (workdir && chdir(workdir->c_str()) != 0) {
      _exit(127);
    }
    FILE* out = std::fopen(out_path.c_str(), "wb");
    FILE* err = std::fopen(err_path.c_str(), "wb");
    if (!out || !err) {
      _exit(127);
    }
    if (dup2(fileno(out), STDOUT_FILENO) == -1 || dup2(fileno(err), STDERR_FILENO) == -1) {
      _exit(127);
    }
    std::fclose(out);
    std::fclose(err);
    execv(argv_ptrs[0], argv_ptrs.data());
    _exit(127);
  }

  const auto deadline = std::chrono::steady_clock::now() + timeout;
  int status = 0;
  while (true) {
    pid_t wait_rc = waitpid(pid, &status, WNOHANG);
    T81_TEST_CHECK(wait_rc != -1);
    if (wait_rc == pid) {
      break;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      std::cerr << "[cli-timeout] " << step.str() << "\n";
      T81_TEST_CHECK(false && "CLI invocation timed out");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }

  CommandResult result;
  result.stdout_text = read_file(out_path);
  result.stderr_text = read_file(err_path);
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.exit_code = 128 + WTERMSIG(status);
  } else {
    result.exit_code = status;
  }

  return result;
}

bool contains(std::string_view text, std::string_view pattern) {
  return text.find(pattern) != std::string_view::npos;
}

bool appears_in_order(std::string_view text, std::initializer_list<std::string_view> patterns) {
  std::size_t cursor = 0;
  for (const auto pattern : patterns) {
    const std::size_t pos = text.find(pattern, cursor);
    if (pos == std::string_view::npos) {
      return false;
    }
    cursor = pos + pattern.size();
  }
  return true;
}

bool contains_any(std::string_view text, const std::vector<std::string>& patterns) {
  for (const auto& pattern : patterns) {
    if (contains(text, pattern)) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> extract_json_string(std::string_view text, std::string_view key) {
  const std::string needle = "\"" + std::string(key) + "\": \"";
  const std::size_t start = text.find(needle);
  if (start == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t value_start = start + needle.size();
  const std::size_t value_end = text.find('"', value_start);
  if (value_end == std::string_view::npos) {
    return std::nullopt;
  }
  return std::string(text.substr(value_start, value_end - value_start));
}

void write_answer_fixed_demo_model(const fs::path& out_dir) {
  std::error_code ec;
  fs::create_directories(out_dir, ec);
  T81_TEST_CHECK(!ec);

  const fs::path model_path = out_dir / "answer-fixed-demo.t81w";
  const fs::path tokenizer_path = out_dir / "tokenizer.json";

  t81::weights::NativeModel model;
  const auto make_tensor = [](std::initializer_list<std::uint32_t> shape, std::uint64_t seed) {
    t81::weights::NativeTensor tensor;
    tensor.shape.assign(shape.begin(), shape.end());
    std::size_t element_count = 1;
    for (std::uint32_t dim : tensor.shape) {
      element_count *= static_cast<std::size_t>(dim);
    }
    tensor.trits = static_cast<std::uint64_t>(element_count);
    tensor.data.resize((element_count + 47u) / 48u, 0u);
    for (std::size_t i = 0; i < tensor.data.size(); ++i) {
      tensor.data[i] = seed + static_cast<std::uint64_t>(i * 17u);
    }
    return tensor;
  };

  model["model.embed_tokens.weight"] = make_tensor({64, 16}, 11u);
  model["model.norm.weight"] = make_tensor({16}, 23u);
  model["model.layers.0.self_attn.q_proj.weight"] = make_tensor({16, 16}, 101u);
  model["model.layers.0.self_attn.k_proj.weight"] = make_tensor({16, 16}, 211u);
  model["model.layers.0.self_attn.v_proj.weight"] = make_tensor({16, 16}, 307u);
  model["model.layers.0.self_attn.o_proj.weight"] = make_tensor({16, 16}, 401u);
  model["model.layers.0.mlp.gate_proj.weight"] = make_tensor({16, 16}, 503u);
  model["model.layers.0.mlp.up_proj.weight"] = make_tensor({16, 16}, 601u);
  model["model.layers.0.mlp.down_proj.weight"] = make_tensor({16, 16}, 701u);
  model["model.layers.1.self_attn.q_proj.weight"] = make_tensor({16, 16}, 809u);
  model["model.layers.1.self_attn.k_proj.weight"] = make_tensor({16, 16}, 907u);
  model["model.layers.1.self_attn.v_proj.weight"] = make_tensor({16, 16}, 1009u);
  model["lm_head.weight"] = make_tensor({64, 16}, 2003u);
  t81::weights::save_t81w(model, model_path);

  std::ofstream out(tokenizer_path);
  T81_TEST_CHECK(static_cast<bool>(out));
  out << R"({
  "model": {
    "type": "BPE",
    "vocab": {
      "NO": 9,
      "greet": 7,
      "hello": 11,
      "world": 12,
      "▁greet": 17,
      "▁hello": 21,
      "YES": 42
    }
  }
}
)";
}

int main(int argc, char* argv[]) {
  T81_TEST_CHECK(argc >= 2);
  const fs::path t81_bin = fs::path(argv[1]);
  T81_TEST_CHECK(fs::exists(t81_bin));
  std::error_code path_ec;
  const fs::path abs_t81_bin = fs::weakly_canonical(t81_bin, path_ec);
  T81_TEST_CHECK(!path_ec);
  const fs::path repo_root = abs_t81_bin.parent_path().parent_path();
  const std::vector<std::string> acceptable_build_dir_fields = {
      "\"build_dir\": \"" + abs_t81_bin.parent_path().string() + "\"",
      "\"build_dir\": \"" + (repo_root / "build").string() + "\""};

  // Test basic CLI functionality - minimal set to avoid hanging
  {
    const auto result = run_cli(t81_bin, {"help", "compile"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Help topic 'compile' has been removed."));
  }

  {
    const fs::path invalid_file = make_temp_path("t81-cli-contract-invalid", ".t81");
    {
      std::ofstream out(invalid_file);
      out << "fn main( {\n";
    }
    const auto result = run_cli(t81_bin, {"code", "check", invalid_file.string()});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, ":1:"));
    T81_TEST_CHECK(contains(result.stderr_text, "^"));
    std::error_code ignore_ec;
    fs::remove(invalid_file, ignore_ec);
  }

  {
    const auto result = run_cli(t81_bin, {"--help"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "code    <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "canonfs <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "determinism <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "vm <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "tisc <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "ir <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "weights <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "policy <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "axion <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "trace <action> [args]"));
    T81_TEST_CHECK(!contains(result.stdout_text, "compile <file.t81|.t81w>"));
    T81_TEST_CHECK(!contains(result.stderr_text, "check   <file.t81>"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "advanced"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "verify/quantize"));
    T81_TEST_CHECK(
        contains(result.stdout_text, "simulate/explain/snapshot/snapshot-diff/rollback"));
    T81_TEST_CHECK(contains(result.stdout_text, "encode/decode"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "benchmark"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "vm|canonfs|weights|determinism"));
  }

  {
    const auto result = run_cli(t81_bin, {"bench"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Command 'bench' has been removed."));
  }

  {
    const auto result = run_cli(t81_bin, {"model", "info", "x"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Command 'model' has been removed."));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "labs"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Runtime-backed memory pool profiling"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "canonfs"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 canonfs <action> [args]"));
    T81_TEST_CHECK(contains(result.stdout_text, "repair"));
    T81_TEST_CHECK(contains(result.stdout_text, "permissive  allow import and export unless an "
                                                "explicit policy evaluator denies the request"));
    T81_TEST_CHECK(contains(
        result.stdout_text,
        "import-only  allow canonfs import and deny canonfs export before materialization"));
    T81_TEST_CHECK(contains(
        result.stdout_text,
        "export-only  allow canonfs export and deny canonfs import before storage writes"));
    T81_TEST_CHECK(contains(
        result.stdout_text,
        "deny-all  deny canonfs import and export before CanonFS or host-side materialization"));
  }

  {
    const auto result =
        run_cli(t81_bin, {"canonfs", "snapshot-diff", "../bad", "../bad", "--json"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stdout_text, "invalid snapshot hash"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "determinism"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 determinism <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "vm"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 vm <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "vm", "trace"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 vm trace"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "determinism", "verify-run"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 determinism verify-run"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "determinism", "explain"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 determinism explain"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "axion", "snapshot-diff"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 axion snapshot-diff"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "trace", "summary"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 trace summary"));
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
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 tisc <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "ir"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Usage: t81 ir <action> [args]"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "code", "profile"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(
        contains(result.stdout_text, "Usage: t81 code profile") ||
        contains(result.stdout_text, "Compile if needed and report VM runtime/memory metrics"));
  }

  {
    const auto result = run_cli(t81_bin, {"env", "doctor", "toolchain", "--json"});
    T81_TEST_CHECK(result.exit_code == 0 || result.exit_code == 2);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.doctor.v1\""));
    T81_TEST_CHECK(contains(result.stdout_text, "\"scope\": \"toolchain\""));
  }

  {
    const auto paths_result = run_cli_in_dir(t81_bin, {"env", "paths", "--json"}, repo_root);
    T81_TEST_CHECK(paths_result.exit_code == 0);
    T81_TEST_CHECK(contains(paths_result.stdout_text, "\"schema\": \"t81.env-paths.v1\""));
    T81_TEST_CHECK(contains_any(paths_result.stdout_text, acceptable_build_dir_fields));

    const auto doctor_result = run_cli_in_dir(t81_bin, {"env", "doctor", "--json"}, repo_root);
    T81_TEST_CHECK(doctor_result.exit_code == 0 || doctor_result.exit_code == 2);
    T81_TEST_CHECK(contains(doctor_result.stdout_text,
                            "\"detail\":\"CLI binary present at " + abs_t81_bin.string() + "\""));
  }

  {
    const auto test_result = run_cli_in_dir(t81_bin, {"code", "test", "--json", "--list"},
                                            fs::absolute(t81_bin).parent_path());
    T81_TEST_CHECK(test_result.exit_code == 0);
    T81_TEST_CHECK(contains(test_result.stdout_text, "\"schema\": \"t81.test.v1\""));
    T81_TEST_CHECK(contains_any(test_result.stdout_text, acceptable_build_dir_fields));
  }

  {
    const auto canonfs_result = run_cli_in_dir(t81_bin, {"canonfs", "snapshot", "--json"},
                                               fs::absolute(t81_bin).parent_path());
    T81_TEST_CHECK(canonfs_result.exit_code == 0);
    T81_TEST_CHECK(contains(canonfs_result.stdout_text, "\"schema\": \"t81.canonfs-snapshot.v1\""));
    T81_TEST_CHECK(contains(canonfs_result.stdout_text,
                            "\"canonfs_root\": \"" + (repo_root / ".t81_canonfs").string() + "\""));
  }

  {
    const fs::path outside_dir = make_temp_path("t81-cli-contract-clean-outside", "");
    std::error_code ignore_ec;
    fs::create_directories(outside_dir, ignore_ec);
    T81_TEST_CHECK(!ignore_ec);
    const auto result =
        run_cli(t81_bin, {"env", "clean", "--build-dir", outside_dir.string(), "--json"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stdout_text, "outside the repository root"));

    const auto force_result = run_cli(
        t81_bin, {"env", "clean", "--build-dir", outside_dir.string(), "--force", "--json"});
    T81_TEST_CHECK(force_result.exit_code == 0);
    T81_TEST_CHECK(contains(force_result.stdout_text, "\"schema\": \"t81.env-clean.v1\""));
    T81_TEST_CHECK(contains(force_result.stdout_text, "\"forced\": true"));
    fs::remove_all(outside_dir, ignore_ec);
  }

  {
    const auto result = run_cli(t81_bin, {"axion", "optimize", "--tier", "nope", "--json"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.error.v1\""));
    T81_TEST_CHECK(contains(result.stdout_text, "--tier must be an integer"));
  }

  {
    const fs::path t81_file = make_temp_path("t81-cli-contract", ".t81");
    const fs::path tisc_file = make_temp_path("t81-cli-contract", ".tisc");
    const fs::path base81_out = make_temp_path("t81-cli-contract", ".base81");
    const fs::path trace_file = make_temp_path("t81-cli-contract", ".trace");
    const fs::path trace_extra = make_temp_path("t81-cli-contract", ".trace-extra");
    const fs::path policy_file = make_temp_path("t81-cli-contract", ".apl");
    const fs::path baseline_dir = make_temp_path("t81-cli-contract-baseline", "");
    const fs::path baseline_source_dir = make_temp_path("t81-cli-contract-source", "");

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

    const auto run_result =
        run_cli(t81_bin, {"code", "run", tisc_file.string(), "-o", trace_file.string()});
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
    const auto diff_result = run_cli(
        t81_bin, {"trace", "diff", trace_file.string(), trace_extra.string(), "--no-color"});
    T81_TEST_CHECK(diff_result.exit_code != 0);
    T81_TEST_CHECK(contains(diff_result.stdout_text, "Difference at line"));

    const auto trace_hash_result =
        run_cli(t81_bin, {"determinism", "trace-hash", trace_file.string(), "--json"});
    T81_TEST_CHECK(trace_hash_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(trace_hash_result.stdout_text, "\"schema\": \"t81.determinism-trace-hash.v1\""));

    const auto trace_show_result =
        run_cli(t81_bin, {"trace", "show", trace_file.string(), "--no-color"});
    T81_TEST_CHECK(trace_show_result.exit_code == 0);
    T81_TEST_CHECK(contains(trace_show_result.stdout_text, "PC="));

    const auto trace_summary_result =
        run_cli(t81_bin, {"trace", "summary", trace_file.string(), "--json"});
    T81_TEST_CHECK(trace_summary_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(trace_summary_result.stdout_text, "\"schema\": \"t81.trace-summary.v1\""));

    const auto canonicalize_result = run_cli(
        t81_bin, {"trace", "canonicalize", trace_file.string(), "-o", trace_extra.string()});
    T81_TEST_CHECK(canonicalize_result.exit_code == 0);
    T81_TEST_CHECK(!read_file(trace_extra).empty());
    T81_TEST_CHECK(contains(read_file(trace_extra), "PC="));

    const auto diff_trace_result = run_cli(
        t81_bin, {"determinism", "diff-trace", trace_file.string(), trace_file.string(), "--json"});
    T81_TEST_CHECK(diff_trace_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(diff_trace_result.stdout_text, "\"schema\": \"t81.determinism-trace-diff.v1\""));

    const auto verify_run_result =
        run_cli(t81_bin, {"determinism", "verify-run", tisc_file.string(), "--json"});
    T81_TEST_CHECK(verify_run_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(verify_run_result.stdout_text, "\"schema\": \"t81.determinism-verify-run.v1\""));

    const auto certify_result =
        run_cli(t81_bin, {"determinism", "certify", tisc_file.string(), "--json"});
    T81_TEST_CHECK(certify_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(certify_result.stdout_text, "\"schema\": \"t81.determinism-certificate.v1\""));
    T81_TEST_CHECK(contains(certify_result.stdout_text, "\"deterministic\": true"));

    const auto explain_result =
        run_cli(t81_bin, {"determinism", "explain", tisc_file.string(), "--json"});
    T81_TEST_CHECK(explain_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(explain_result.stdout_text, "\"schema\": \"t81.determinism-explain.v1\""));

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

    const auto vm_until_result = run_cli(
        t81_bin, {"vm", "until", tisc_file.string(), "--pc", "0", "--steps", "1", "--json"});
    T81_TEST_CHECK(vm_until_result.exit_code == 0 || vm_until_result.exit_code == 1);
    T81_TEST_CHECK(contains(vm_until_result.stdout_text, "\"schema\": \"t81.vm-until.v1\""));

    const auto vm_regs_result =
        run_cli(t81_bin, {"vm", "regs", tisc_file.string(), "--steps", "2", "--json"});
    T81_TEST_CHECK(vm_regs_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_regs_result.stdout_text, "\"schema\": \"t81.vm-regs.v1\""));

    const auto vm_stack_result =
        run_cli(t81_bin, {"vm", "stack", tisc_file.string(), "--steps", "2", "--json"});
    T81_TEST_CHECK(vm_stack_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_stack_result.stdout_text, "\"schema\": \"t81.vm-stack.v1\""));

    const auto vm_mem_result = run_cli(
        t81_bin, {"vm", "mem", tisc_file.string(), "--addr", "0", "--steps", "2", "--json"});
    T81_TEST_CHECK(vm_mem_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_mem_result.stdout_text, "\"schema\": \"t81.vm-mem.v1\""));

    const auto vm_state_result = run_cli(t81_bin, {"vm", "state", tisc_file.string(), "--json"});
    T81_TEST_CHECK(vm_state_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_state_result.stdout_text, "\"schema\": \"t81.vm-state.v1\""));

    const auto vm_profile_result =
        run_cli(t81_bin, {"vm", "profile", tisc_file.string(), "--json"});
    T81_TEST_CHECK(vm_profile_result.exit_code == 0);
    T81_TEST_CHECK(contains(vm_profile_result.stdout_text, "\"schema\": \"t81.vm-profile.v1\""));

    const auto tisc_validate_result =
        run_cli(t81_bin, {"tisc", "validate", tisc_file.string(), "--json"});
    T81_TEST_CHECK(tisc_validate_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(tisc_validate_result.stdout_text, "\"schema\": \"t81.tisc-validate.v1\""));

    const auto tisc_decode_result = run_cli(
        t81_bin, {"tisc", "decode", tisc_file.string(), "-o", base81_out.string(), "--json"});
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

    const auto lang_validate_result =
        run_cli(t81_bin, {"lang", "validate", t81_file.string(), "--json"});
    T81_TEST_CHECK(lang_validate_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(lang_validate_result.stdout_text, "\"schema\": \"t81.ir-validate.v1\""));

    const auto policy_test_result = run_cli(
        t81_bin,
        {"policy", "test", policy_file.string(), "--model-hash", "sha3-256:test-hash", "--json"});
    T81_TEST_CHECK(policy_test_result.exit_code == 0);
    T81_TEST_CHECK(contains(policy_test_result.stdout_text, "\"schema\": \"t81.policy-test.v1\""));

    const fs::path denied_policy_file = make_temp_path("t81-cli-policy-deny", ".apl");
    {
      std::ofstream out(denied_policy_file);
      out << "(policy\n"
             "  (tier 1)\n"
             "  (allowed-tensor-hashes [\"sha3-256:other-hash\"]))\n";
    }
    const auto policy_deny_result =
        run_cli(t81_bin, {"policy", "test", denied_policy_file.string(), "--model-hash",
                          "sha3-256:test-hash", "--json"});
    T81_TEST_CHECK(policy_deny_result.exit_code == 1);
    T81_TEST_CHECK(contains(policy_deny_result.stdout_text, "\"schema\": \"t81.policy-test.v1\""));
    T81_TEST_CHECK(contains(policy_deny_result.stdout_text, "\"ok\": false"));
    T81_TEST_CHECK(contains(policy_deny_result.stdout_text, "\"rule\": \"hash_not_allowed\""));

    const auto axion_explain_result =
        run_cli(t81_bin, {"axion", "explain", policy_file.string(), "--json"});
    T81_TEST_CHECK(axion_explain_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(axion_explain_result.stdout_text, "\"schema\":\"t81.axion-explain.v1\""));

    const auto axion_status_result = run_cli(t81_bin, {"axion", "status", "--json"});
    T81_TEST_CHECK(axion_status_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(axion_status_result.stdout_text, "\"schema\": \"t81.axion-status.v1\""));
    T81_TEST_CHECK(contains(axion_status_result.stdout_text, "\"issues\": ["));

    const auto axion_log_result = run_cli(t81_bin, {"axion", "log", "--tail", "1", "--json"});
    T81_TEST_CHECK(axion_log_result.exit_code == 0);
    T81_TEST_CHECK(contains(axion_log_result.stdout_text, "\"schema\": \"t81.axion-log.v1\""));
    T81_TEST_CHECK(contains(axion_log_result.stdout_text, "\"tail\": 1"));
    T81_TEST_CHECK(
        contains(axion_log_result.stdout_text, "\"receipt_persistence\": \"not_persisted\""));

    const auto axion_optimize_result =
        run_cli(t81_bin, {"axion", "optimize", "--tier", "2", "--json"});
    T81_TEST_CHECK(axion_optimize_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(axion_optimize_result.stdout_text, "\"schema\": \"t81.axion-optimize.v1\""));
    T81_TEST_CHECK(contains(axion_optimize_result.stdout_text, "\"only_current_count\":"));

    const auto axion_snapshot_result = run_cli(t81_bin, {"axion", "snapshot", "--json"});
    T81_TEST_CHECK(axion_snapshot_result.exit_code == 0);
    const auto axion_hash_pos = axion_snapshot_result.stdout_text.find("\"hash\": \"");
    T81_TEST_CHECK(axion_hash_pos != std::string::npos);
    const auto axion_hash_start = axion_hash_pos + std::string("\"hash\": \"").size();
    const auto axion_hash_end = axion_snapshot_result.stdout_text.find('"', axion_hash_start);
    T81_TEST_CHECK(axion_hash_end != std::string::npos);
    const std::string axion_snapshot_hash = axion_snapshot_result.stdout_text.substr(
        axion_hash_start, axion_hash_end - axion_hash_start);

    const auto axion_snapshot_diff_result = run_cli(
        t81_bin, {"axion", "snapshot-diff", axion_snapshot_hash, axion_snapshot_hash, "--json"});
    T81_TEST_CHECK(axion_snapshot_diff_result.exit_code == 0);
    T81_TEST_CHECK(contains(axion_snapshot_diff_result.stdout_text,
                            "\"schema\": \"t81.canonfs-snapshot-diff.v1\""));

    const auto axion_audit_result = run_cli(t81_bin, {"axion", "audit", "--json"});
    T81_TEST_CHECK(axion_audit_result.exit_code == 0);
    T81_TEST_CHECK(contains(axion_audit_result.stdout_text, "\"schema\": \"t81.axion-audit.v1\""));
    T81_TEST_CHECK(
        contains(axion_audit_result.stdout_text, "\"receipt_persistence\": \"not_persisted\""));

    const auto weights_verify_help = run_cli(t81_bin, {"help", "weights", "verify"});
    T81_TEST_CHECK(weights_verify_help.exit_code == 0);
    T81_TEST_CHECK(contains(weights_verify_help.stdout_text, "Usage: t81 weights verify"));

    const auto lang_help = run_cli(t81_bin, {"help", "lang"});
    T81_TEST_CHECK(lang_help.exit_code == 0);
    T81_TEST_CHECK(contains(lang_help.stdout_text, "Usage: t81 lang <action>"));

    const auto lang_export_result =
        run_cli(t81_bin, {"lang", "export", t81_file.string(), "--json"});
    T81_TEST_CHECK(lang_export_result.exit_code == 0);
    T81_TEST_CHECK(contains(lang_export_result.stdout_text, "\"schema\": \"t81.ir-export.v1\""));

    const auto profile_result = run_cli(t81_bin, {"code", "profile", t81_file.string(), "--json"});
    T81_TEST_CHECK(profile_result.exit_code == 0);
    T81_TEST_CHECK(contains(profile_result.stdout_text, "\"schema\": \"t81.vm-profile.v1\""));

    const auto tensor_help = run_cli(t81_bin, {"help", "tensor", "hash"});
    T81_TEST_CHECK(tensor_help.exit_code == 0);
    T81_TEST_CHECK(contains(tensor_help.stdout_text, "Usage: t81 tensor hash"));

    const auto policy_validate_result =
        run_cli(t81_bin, {"policy", "validate", policy_file.string(), "--json"});
    T81_TEST_CHECK(policy_validate_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(policy_validate_result.stdout_text, "\"schema\": \"t81.policy-validate.v1\""));

    const fs::path invalid_policy_file = make_temp_path("t81-cli-policy-invalid", ".apl");
    {
      std::ofstream out(invalid_policy_file);
      out << "(policy (tier 1) (unknown-clause 1))\n";
    }
    const auto invalid_policy_validate_result =
        run_cli(t81_bin, {"policy", "validate", invalid_policy_file.string(), "--json"});
    T81_TEST_CHECK(invalid_policy_validate_result.exit_code == 1);
    T81_TEST_CHECK(contains(invalid_policy_validate_result.stdout_text,
                            "\"schema\": \"t81.policy-validate.v1\""));
    T81_TEST_CHECK(contains(invalid_policy_validate_result.stdout_text, "\"valid\": false"));
    T81_TEST_CHECK(
        contains(invalid_policy_validate_result.stdout_text, "\"error\": \"Policy parse error:"));

    const auto invalid_policy_run_result =
        run_cli(t81_bin, {"policy", "run", invalid_policy_file.string(), "--json"});
    T81_TEST_CHECK(invalid_policy_run_result.exit_code == 1);
    T81_TEST_CHECK(
        contains(invalid_policy_run_result.stdout_text, "\"schema\": \"t81.policy-run.v1\""));
    T81_TEST_CHECK(contains(invalid_policy_run_result.stdout_text, "\"valid\": false"));
    T81_TEST_CHECK(
        contains(invalid_policy_run_result.stdout_text, "\"error\": \"Policy parse error:"));

    std::error_code baseline_ec;
    fs::create_directories(baseline_dir, baseline_ec);
    T81_TEST_CHECK(!baseline_ec);
    fs::create_directories(baseline_source_dir, baseline_ec);
    T81_TEST_CHECK(!baseline_ec);
    fs::copy_file(tisc_file, baseline_source_dir / tisc_file.filename(),
                  fs::copy_options::overwrite_existing, baseline_ec);
    T81_TEST_CHECK(!baseline_ec);
    const auto baseline_result =
        run_cli(t81_bin, {"determinism", "baseline", baseline_dir.string(), "--source-dir",
                          baseline_source_dir.string(), "--json"});
    T81_TEST_CHECK(baseline_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(baseline_result.stdout_text, "\"schema\": \"t81.determinism-baseline.v1\""));
    const auto baseline_verify_result =
        run_cli(t81_bin, {"determinism", "verify", baseline_dir.string(), "--json"});
    T81_TEST_CHECK(baseline_verify_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(baseline_verify_result.stdout_text, "\"schema\": \"t81.determinism-verify.v1\""));

    const auto compare_run_result =
        run_cli(t81_bin, {"determinism", "compare-run", tisc_file.string(), "--json"});
    T81_TEST_CHECK(compare_run_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(compare_run_result.stdout_text, "\"schema\": \"t81.determinism-compare-run.v1\""));

    const auto bisect_result =
        run_cli(t81_bin, {"determinism", "bisect", baseline_dir.string(), "--json"});
    T81_TEST_CHECK(bisect_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(bisect_result.stdout_text, "\"schema\": \"t81.determinism-bisect.v1\""));

    std::error_code ignore_ec;
    fs::remove(t81_file, ignore_ec);
    fs::remove(tisc_file, ignore_ec);
    fs::remove(base81_out, ignore_ec);
    fs::remove(trace_file, ignore_ec);
    fs::remove(trace_extra, ignore_ec);
    fs::remove(policy_file, ignore_ec);
    fs::remove(denied_policy_file, ignore_ec);
    fs::remove(invalid_policy_file, ignore_ec);
    fs::remove_all(baseline_dir, ignore_ec);
    fs::remove_all(baseline_source_dir, ignore_ec);
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

    std::error_code ignore_ec;
    const auto import_file_result =
        run_cli(t81_bin, {"canonfs", "import", payload.string(), "--json"});
    T81_TEST_CHECK(import_file_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(import_file_result.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(contains(import_file_result.stdout_text, "\"source_kind\": \"host-file\""));
    T81_TEST_CHECK(contains(import_file_result.stdout_text,
                            "\"provenance_schema\": \"t81.canonfs-import-provenance.v1\""));
    T81_TEST_CHECK(contains(import_file_result.stdout_text,
                            "\"manifest_schema\": \"t81.canonfs-interchange-manifest.v1\""));
    const auto imported_file_ref =
        extract_json_string(import_file_result.stdout_text, "manifest_ref");
    T81_TEST_CHECK(imported_file_ref.has_value());

    const fs::path imported_file_restore = make_temp_path("t81-cli-contract", ".imported");
    const auto export_file_result = run_cli(
        t81_bin, {"canonfs", "export", hash, "--out", imported_file_restore.string(), "--json"});
    T81_TEST_CHECK(export_file_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(export_file_result.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(contains(export_file_result.stdout_text, "\"target_kind\": \"host-file\""));
    T81_TEST_CHECK(contains(export_file_result.stdout_text,
                            "\"provenance_schema\": \"t81.canonfs-export-provenance.v1\""));
    T81_TEST_CHECK(read_file(imported_file_restore) == "canonfs-contract");

    const fs::path missing_import_path = make_temp_path("t81-cli-contract-missing-import", ".txt");
    const auto import_missing_result =
        run_cli(t81_bin, {"canonfs", "import", missing_import_path.string(), "--json"});
    T81_TEST_CHECK(import_missing_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(import_missing_result.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(contains(import_missing_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(import_missing_result.stdout_text, "\"kind\": \"source-failure\""));
    T81_TEST_CHECK(
        contains(import_missing_result.stdout_text, "\"code\": \"canonfs-import-missing-source\""));
    T81_TEST_CHECK(contains(import_missing_result.stdout_text, "\"reason\": \"missing_source\""));

    const auto export_missing_result = run_cli(
        t81_bin, {"canonfs", "export", "sha3-256:11111111111111111111111111111111111111111111",
                  "--out", imported_file_restore.string(), "--json"});
    T81_TEST_CHECK(export_missing_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(export_missing_result.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(contains(export_missing_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(
        contains(export_missing_result.stdout_text, "\"kind\": \"materialization-failure\""));
    T81_TEST_CHECK(contains(export_missing_result.stdout_text,
                            "\"code\": \"canonfs-export-invalid-source-ref\""));
    T81_TEST_CHECK(
        contains(export_missing_result.stdout_text, "\"reason\": \"invalid_source_ref\""));

    const fs::path import_tree = make_temp_path("t81-cli-contract-tree", "");
    fs::create_directories(import_tree / "nested", ignore_ec);
    T81_TEST_CHECK(!ignore_ec);
    {
      std::ofstream out(import_tree / "alpha.txt");
      out << "alpha";
    }
    {
      std::ofstream out(import_tree / "nested" / "beta.txt");
      out << "beta";
    }
    const auto import_tree_result =
        run_cli(t81_bin, {"canonfs", "import", import_tree.string(), "--json"});
    T81_TEST_CHECK(import_tree_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(import_tree_result.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(contains(import_tree_result.stdout_text, "\"source_kind\": \"host-directory\""));
    const auto manifest_ref = extract_json_string(import_tree_result.stdout_text, "manifest_ref");
    T81_TEST_CHECK(manifest_ref.has_value());

    const fs::path export_tree = make_temp_path("t81-cli-contract-tree-export", "");
    const auto export_tree_result = run_cli(
        t81_bin, {"canonfs", "export", *manifest_ref, "--out", export_tree.string(), "--json"});
    T81_TEST_CHECK(export_tree_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(export_tree_result.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(contains(export_tree_result.stdout_text, "\"target_kind\": \"host-directory\""));
    T81_TEST_CHECK(read_file(export_tree / "alpha.txt") == "alpha");
    T81_TEST_CHECK(read_file(export_tree / "nested" / "beta.txt") == "beta");

    const fs::path malformed_manifest = make_temp_path("t81-cli-contract-bad-manifest", ".json");
    {
      std::ofstream out(malformed_manifest);
      out << "{\n"
             "  \"schema\": \"t81.canonfs-interchange-manifest.v1\",\n"
             "  \"entries\": []\n"
             "}\n";
    }
    const auto malformed_manifest_put =
        run_cli(t81_bin, {"canonfs", "put-file", malformed_manifest.string()});
    T81_TEST_CHECK(malformed_manifest_put.exit_code == 0);
    std::string malformed_manifest_hash = malformed_manifest_put.stdout_text;
    while (!malformed_manifest_hash.empty() &&
           std::isspace(static_cast<unsigned char>(malformed_manifest_hash.back()))) {
      malformed_manifest_hash.pop_back();
    }
    const fs::path malformed_export_target = make_temp_path("t81-cli-contract-bad-export", "");
    const auto malformed_export_result =
        run_cli(t81_bin, {"canonfs", "export", malformed_manifest_hash, "--out",
                          malformed_export_target.string(), "--json"});
    T81_TEST_CHECK(malformed_export_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(malformed_export_result.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(contains(malformed_export_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(malformed_export_result.stdout_text, "invalid interchange manifest"));
    T81_TEST_CHECK(
        contains(malformed_export_result.stdout_text, "\"kind\": \"materialization-failure\""));
    T81_TEST_CHECK(contains(malformed_export_result.stdout_text,
                            "\"code\": \"canonfs-export-malformed-manifest\""));
    T81_TEST_CHECK(
        contains(malformed_export_result.stdout_text, "\"reason\": \"malformed_manifest\""));

    const fs::path invalid_schema_object = make_temp_path("t81-cli-contract-bad-schema", ".json");
    {
      std::ofstream out(invalid_schema_object);
      out << "{\n"
             "  \"schema\": \"t81.canonfs-unknown.v1\",\n"
             "  \"payload\": \"not-a-manifest\"\n"
             "}\n";
    }
    const auto invalid_schema_put =
        run_cli(t81_bin, {"canonfs", "put-file", invalid_schema_object.string()});
    T81_TEST_CHECK(invalid_schema_put.exit_code == 0);
    std::string invalid_schema_hash = invalid_schema_put.stdout_text;
    while (!invalid_schema_hash.empty() &&
           std::isspace(static_cast<unsigned char>(invalid_schema_hash.back()))) {
      invalid_schema_hash.pop_back();
    }
    const auto invalid_schema_export =
        run_cli(t81_bin,
                {"canonfs", "export", invalid_schema_hash, "--out",
                 make_temp_path("t81-cli-contract-invalid-schema-out", ".bin").string(), "--json"});
    T81_TEST_CHECK(invalid_schema_export.exit_code != 0);
    T81_TEST_CHECK(
        contains(invalid_schema_export.stdout_text, "\"code\": \"canonfs-export-invalid-schema\""));
    T81_TEST_CHECK(contains(invalid_schema_export.stdout_text, "\"reason\": \"invalid_schema\""));

    const fs::path tampered_root = make_temp_path("t81-cli-contract-tampered-root", "");
    fs::create_directories(tampered_root, ignore_ec);
    T81_TEST_CHECK(!ignore_ec);
    const auto tampered_put = run_cli(t81_bin, {"canonfs", "put-file", payload.string(),
                                                "--canonfs-root", tampered_root.string()});
    T81_TEST_CHECK(tampered_put.exit_code == 0);
    std::string tampered_hash = tampered_put.stdout_text;
    while (!tampered_hash.empty() &&
           std::isspace(static_cast<unsigned char>(tampered_hash.back()))) {
      tampered_hash.pop_back();
    }
    const fs::path tampered_object =
        tampered_root / "objects" /
        (tampered_hash.substr(std::string("sha3-256:").size()) + ".blk");
    const fs::path tampered_source_two = make_temp_path("t81-cli-contract-tampered-other", ".txt");
    {
      std::ofstream out(tampered_source_two);
      out << "tampered-cli-contract-object-two";
    }
    const auto tampered_put_two =
        run_cli(t81_bin, {"canonfs", "put-file", tampered_source_two.string(), "--canonfs-root",
                          tampered_root.string()});
    T81_TEST_CHECK(tampered_put_two.exit_code == 0);
    std::string tampered_hash_two = tampered_put_two.stdout_text;
    while (!tampered_hash_two.empty() &&
           std::isspace(static_cast<unsigned char>(tampered_hash_two.back()))) {
      tampered_hash_two.pop_back();
    }
    const fs::path tampered_object_two =
        tampered_root / "objects" /
        (tampered_hash_two.substr(std::string("sha3-256:").size()) + ".blk");
    fs::copy_file(tampered_object_two, tampered_object, fs::copy_options::overwrite_existing,
                  ignore_ec);
    T81_TEST_CHECK(!ignore_ec);
    const auto tampered_export_result = run_cli(
        t81_bin,
        {"canonfs", "export", tampered_hash, "--canonfs-root", tampered_root.string(), "--out",
         make_temp_path("t81-cli-contract-tampered-out", ".bin").string(), "--json"});
    T81_TEST_CHECK(tampered_export_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(tampered_export_result.stdout_text, "\"code\": \"canonfs-export-hash-mismatch\""));
    T81_TEST_CHECK(contains(tampered_export_result.stdout_text, "\"reason\": \"hash_mismatch\""));

    const fs::path canonfs_policy = make_temp_path("t81-cli-contract-canonfs-policy", ".apl");
    {
      std::ofstream out(canonfs_policy);
      out << "(policy (tier 1) (allowed-tensor-hashes []))\n";
    }
    const auto import_policy_deny_result = run_cli(
        t81_bin,
        {"canonfs", "import", payload.string(), "--policy", canonfs_policy.string(), "--json"});
    T81_TEST_CHECK(import_policy_deny_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(import_policy_deny_result.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(
        contains(import_policy_deny_result.stdout_text, "\"policy_result\": \"denied\""));
    T81_TEST_CHECK(
        contains(import_policy_deny_result.stdout_text, "\"policy_profile\": \"permissive\""));
    T81_TEST_CHECK(contains(import_policy_deny_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(import_policy_deny_result.stdout_text, "\"kind\": \"policy-failure\""));
    T81_TEST_CHECK(
        contains(import_policy_deny_result.stdout_text, "\"code\": \"canonfs-policy-denied\""));
    T81_TEST_CHECK(
        contains(import_policy_deny_result.stdout_text, "\"reason\": \"policy_denied\""));

    const auto export_policy_deny_result =
        run_cli(t81_bin, {"canonfs", "export", hash, "--policy", canonfs_policy.string(), "--out",
                          imported_file_restore.string(), "--json"});
    T81_TEST_CHECK(export_policy_deny_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(export_policy_deny_result.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(
        contains(export_policy_deny_result.stdout_text, "\"policy_result\": \"denied\""));
    T81_TEST_CHECK(
        contains(export_policy_deny_result.stdout_text, "\"policy_profile\": \"permissive\""));
    T81_TEST_CHECK(contains(export_policy_deny_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(export_policy_deny_result.stdout_text, "\"kind\": \"policy-failure\""));
    T81_TEST_CHECK(
        contains(export_policy_deny_result.stdout_text, "\"code\": \"canonfs-policy-denied\""));
    T81_TEST_CHECK(
        contains(export_policy_deny_result.stdout_text, "\"reason\": \"policy_denied\""));

    const auto import_profile_deny_result = run_cli(
        t81_bin,
        {"canonfs", "import", payload.string(), "--policy-profile", "export-only", "--json"});
    T81_TEST_CHECK(import_profile_deny_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(import_profile_deny_result.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(
        contains(import_profile_deny_result.stdout_text, "\"policy_result\": \"denied\""));
    T81_TEST_CHECK(
        contains(import_profile_deny_result.stdout_text, "\"policy_profile\": \"export-only\""));
    T81_TEST_CHECK(contains(import_profile_deny_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(import_profile_deny_result.stdout_text,
                            "policy-profile export-only denies canonfs.import before CanonFS "
                            "storage writes"));

    const auto export_profile_deny_result =
        run_cli(t81_bin, {"canonfs", "export", hash, "--policy-profile", "import-only", "--out",
                          imported_file_restore.string(), "--json"});
    T81_TEST_CHECK(export_profile_deny_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(export_profile_deny_result.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(
        contains(export_profile_deny_result.stdout_text, "\"policy_result\": \"denied\""));
    T81_TEST_CHECK(
        contains(export_profile_deny_result.stdout_text, "\"policy_profile\": \"import-only\""));
    T81_TEST_CHECK(contains(export_profile_deny_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(export_profile_deny_result.stdout_text,
                            "policy-profile import-only denies canonfs.export before host-side "
                            "materialization"));

    const auto deny_all_import_result = run_cli(
        t81_bin, {"canonfs", "import", payload.string(), "--policy-profile", "deny-all", "--json"});
    T81_TEST_CHECK(deny_all_import_result.exit_code != 0);
    T81_TEST_CHECK(contains(deny_all_import_result.stdout_text, "\"policy_result\": \"denied\""));
    T81_TEST_CHECK(
        contains(deny_all_import_result.stdout_text, "\"policy_profile\": \"deny-all\""));
    T81_TEST_CHECK(contains(deny_all_import_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(deny_all_import_result.stdout_text, "\"reason\": \"policy_denied\""));
    T81_TEST_CHECK(contains(deny_all_import_result.stdout_text,
                            "policy-profile deny-all denies canonfs.import before CanonFS "
                            "storage writes"));

    const auto deny_all_export_result =
        run_cli(t81_bin, {"canonfs", "export", hash, "--policy-profile", "deny-all", "--out",
                          imported_file_restore.string(), "--json"});
    T81_TEST_CHECK(deny_all_export_result.exit_code != 0);
    T81_TEST_CHECK(contains(deny_all_export_result.stdout_text, "\"policy_result\": \"denied\""));
    T81_TEST_CHECK(
        contains(deny_all_export_result.stdout_text, "\"policy_profile\": \"deny-all\""));
    T81_TEST_CHECK(contains(deny_all_export_result.stdout_text, "\"status\": \"error\""));
    T81_TEST_CHECK(contains(deny_all_export_result.stdout_text, "\"reason\": \"policy_denied\""));
    T81_TEST_CHECK(contains(deny_all_export_result.stdout_text,
                            "policy-profile deny-all denies canonfs.export before host-side "
                            "materialization"));

    const auto invalid_profile_result = run_cli(
        t81_bin, {"canonfs", "import", payload.string(), "--policy-profile", "bogus", "--json"});
    T81_TEST_CHECK(invalid_profile_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(invalid_profile_result.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(contains(invalid_profile_result.stdout_text, "invalid policy profile"));
    T81_TEST_CHECK(contains(invalid_profile_result.stdout_text,
                            "\"code\": \"canonfs-import-invalid-policy-profile\""));
    T81_TEST_CHECK(
        contains(invalid_profile_result.stdout_text, "\"reason\": \"invalid_policy_profile\""));

    const fs::path example_input_dir =
        repo_root / "examples" / "storage-and-canonfs" / "canonfs-interchange" / "input";
    const fs::path example_deny_policy = repo_root / "examples" / "storage-and-canonfs" /
                                         "canonfs-interchange" / "policy-deny-all.apl";
    const fs::path example_allow_policy = repo_root / "examples" / "storage-and-canonfs" /
                                          "canonfs-interchange" / "policy-allow-example-inputs.apl";
    const fs::path example_alpha_only_policy = repo_root / "examples" / "storage-and-canonfs" /
                                               "canonfs-interchange" /
                                               "policy-allow-alpha-only.apl";
    T81_TEST_CHECK(fs::exists(example_input_dir));
    T81_TEST_CHECK(fs::exists(example_deny_policy));
    T81_TEST_CHECK(fs::exists(example_allow_policy));
    T81_TEST_CHECK(fs::exists(example_alpha_only_policy));

    const auto validate_deny_example =
        run_cli(t81_bin, {"policy", "validate", example_deny_policy.string(), "--json"});
    T81_TEST_CHECK(validate_deny_example.exit_code == 0);
    T81_TEST_CHECK(
        contains(validate_deny_example.stdout_text, "\"schema\": \"t81.policy-validate.v1\""));
    T81_TEST_CHECK(contains(validate_deny_example.stdout_text, "\"valid\": true"));

    const auto validate_alpha_only_example =
        run_cli(t81_bin, {"policy", "validate", example_alpha_only_policy.string(), "--json"});
    T81_TEST_CHECK(validate_alpha_only_example.exit_code == 0);
    T81_TEST_CHECK(contains(validate_alpha_only_example.stdout_text,
                            "\"schema\": \"t81.policy-validate.v1\""));
    T81_TEST_CHECK(contains(validate_alpha_only_example.stdout_text, "\"valid\": true"));

    const auto validate_allow_example =
        run_cli(t81_bin, {"policy", "validate", example_allow_policy.string(), "--json"});
    T81_TEST_CHECK(validate_allow_example.exit_code == 0);
    T81_TEST_CHECK(
        contains(validate_allow_example.stdout_text, "\"schema\": \"t81.policy-validate.v1\""));
    T81_TEST_CHECK(contains(validate_allow_example.stdout_text, "\"valid\": true"));

    const fs::path example_canon_root = make_temp_path("t81-cli-canonfs-example-root", "");
    const auto example_policy_deny_import = run_cli(
        t81_bin, {"canonfs", "import", example_input_dir.string(), "--canonfs-root",
                  example_canon_root.string(), "--policy", example_deny_policy.string(), "--json"});
    T81_TEST_CHECK(example_policy_deny_import.exit_code != 0);
    T81_TEST_CHECK(
        contains(example_policy_deny_import.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(
        contains(example_policy_deny_import.stdout_text, "\"policy_result\": \"denied\""));
    T81_TEST_CHECK(
        contains(example_policy_deny_import.stdout_text, "\"reason\": \"policy_denied\""));

    const auto example_policy_allow_import =
        run_cli(t81_bin,
                {"canonfs", "import", example_input_dir.string(), "--canonfs-root",
                 example_canon_root.string(), "--policy", example_allow_policy.string(), "--json"});
    T81_TEST_CHECK(example_policy_allow_import.exit_code == 0);
    T81_TEST_CHECK(
        contains(example_policy_allow_import.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(contains(example_policy_allow_import.stdout_text, "\"status\": \"ok\""));
    const auto example_manifest_ref =
        extract_json_string(example_policy_allow_import.stdout_text, "manifest_ref");
    T81_TEST_CHECK(example_manifest_ref.has_value());

    const fs::path example_export_root = make_temp_path("t81-cli-canonfs-example-export", "");
    const auto example_policy_allow_export =
        run_cli(t81_bin, {"canonfs", "export", *example_manifest_ref, "--canonfs-root",
                          example_canon_root.string(), "--policy", example_allow_policy.string(),
                          "--out", example_export_root.string(), "--json"});
    T81_TEST_CHECK(example_policy_allow_export.exit_code == 0);
    T81_TEST_CHECK(
        contains(example_policy_allow_export.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(contains(example_policy_allow_export.stdout_text, "\"status\": \"ok\""));
    T81_TEST_CHECK(fs::exists(example_export_root / "alpha.txt"));
    T81_TEST_CHECK(fs::exists(example_export_root / "nested" / "beta.txt"));

    const fs::path example_alpha_only_root =
        make_temp_path("t81-cli-canonfs-example-alpha-root", "");
    const auto example_alpha_only_import =
        run_cli(t81_bin, {"canonfs", "import", example_input_dir.string(), "--canonfs-root",
                          example_alpha_only_root.string(), "--policy",
                          example_alpha_only_policy.string(), "--json"});
    T81_TEST_CHECK(example_alpha_only_import.exit_code == 0);
    T81_TEST_CHECK(
        contains(example_alpha_only_import.stdout_text, "\"schema\": \"t81.canonfs-import.v1\""));
    T81_TEST_CHECK(contains(example_alpha_only_import.stdout_text, "\"status\": \"partial\""));
    T81_TEST_CHECK(
        contains(example_alpha_only_import.stdout_text, "\"policy_result\": \"partial\""));
    T81_TEST_CHECK(contains(example_alpha_only_import.stdout_text, "\"alpha.txt\""));
    T81_TEST_CHECK(
        contains(example_alpha_only_import.stdout_text, "\"reason\": \"policy_denied\""));
    T81_TEST_CHECK(contains(example_alpha_only_import.stdout_text, "nested/beta.txt"));

    const auto example_full_import_for_partial_export =
        run_cli(t81_bin, {"canonfs", "import", example_input_dir.string(), "--canonfs-root",
                          example_alpha_only_root.string(), "--json"});
    T81_TEST_CHECK(example_full_import_for_partial_export.exit_code == 0);
    const auto example_full_manifest_ref =
        extract_json_string(example_full_import_for_partial_export.stdout_text, "manifest_ref");
    T81_TEST_CHECK(example_full_manifest_ref.has_value());

    const fs::path example_alpha_only_export_root =
        make_temp_path("t81-cli-canonfs-example-alpha-export", "");
    const auto example_alpha_only_export = run_cli(
        t81_bin, {"canonfs", "export", *example_full_manifest_ref, "--canonfs-root",
                  example_alpha_only_root.string(), "--policy", example_alpha_only_policy.string(),
                  "--out", example_alpha_only_export_root.string(), "--json"});
    T81_TEST_CHECK(example_alpha_only_export.exit_code == 0);
    T81_TEST_CHECK(
        contains(example_alpha_only_export.stdout_text, "\"schema\": \"t81.canonfs-export.v1\""));
    T81_TEST_CHECK(contains(example_alpha_only_export.stdout_text, "\"status\": \"partial\""));
    T81_TEST_CHECK(
        contains(example_alpha_only_export.stdout_text, "\"policy_result\": \"partial\""));
    T81_TEST_CHECK(
        contains(example_alpha_only_export.stdout_text, "\"reason\": \"policy_denied\""));
    T81_TEST_CHECK(contains(example_alpha_only_export.stdout_text, "nested/beta.txt"));
    T81_TEST_CHECK(fs::exists(example_alpha_only_export_root / "alpha.txt"));
    T81_TEST_CHECK(!fs::exists(example_alpha_only_export_root / "nested" / "beta.txt"));

    const fs::path contract_root = repo_root / "build" / "canonfs-v1-contract-root";
    const fs::path contract_export_dir = repo_root / "build" / "canonfs-v1-contract-export";
    std::error_code contract_ec;
    fs::remove_all(contract_root, contract_ec);
    contract_ec.clear();
    fs::remove_all(contract_export_dir, contract_ec);
    contract_ec.clear();
    fs::create_directories(contract_export_dir, contract_ec);
    T81_TEST_CHECK(!contract_ec);

    const fs::path v1_dir =
        repo_root / "examples" / "storage-and-canonfs" / "canonfs-interchange" / "v1";
    const fs::path v1_model = v1_dir / "model.t81w";
    const fs::path v1_invalid_schema = v1_dir / "invalid-schema.json";
    const fs::path v1_policy = v1_dir / "policy-deny-all.apl";
    const fs::path v1_import_output = v1_dir / "import-success.output.json";
    const fs::path v1_export_output = v1_dir / "export-success.output.json";
    const fs::path v1_missing_output = v1_dir / "export-missing-object.output.json";
    const fs::path v1_invalid_schema_output = v1_dir / "export-invalid-schema.output.json";
    const fs::path v1_policy_deny_output = v1_dir / "import-policy-denied.output.json";
    T81_TEST_CHECK(fs::exists(v1_model));
    T81_TEST_CHECK(fs::exists(v1_invalid_schema));
    T81_TEST_CHECK(fs::exists(v1_policy));

    const auto v1_import_result = run_cli_in_dir(
        t81_bin,
        {"canonfs", "import", "examples/storage-and-canonfs/canonfs-interchange/v1/model.t81w",
         "--canonfs-root", "build/canonfs-v1-contract-root", "--json"},
        repo_root);
    T81_TEST_CHECK(v1_import_result.exit_code == 0);
    T81_TEST_CHECK(read_file(v1_import_output) == v1_import_result.stdout_text);

    const auto v1_export_result =
        run_cli_in_dir(t81_bin,
                       {"canonfs", "export", "sha3-256:5AnZIω3JoaS7π≠5MG7K>cy3goOKHUEudxccikkcsX",
                        "--canonfs-root", "build/canonfs-v1-contract-root", "--out",
                        "build/canonfs-v1-contract-export/model.t81w", "--json"},
                       repo_root);
    T81_TEST_CHECK(v1_export_result.exit_code == 0);
    T81_TEST_CHECK(read_file(v1_export_output) == v1_export_result.stdout_text);

    const auto v1_missing_object_result =
        run_cli_in_dir(t81_bin,
                       {"canonfs", "export", "sha3-256:missing", "--canonfs-root",
                        "build/canonfs-v1-contract-root", "--out",
                        "build/canonfs-v1-contract-export/missing.bin", "--json"},
                       repo_root);
    T81_TEST_CHECK(v1_missing_object_result.exit_code != 0);
    T81_TEST_CHECK(read_file(v1_missing_output) == v1_missing_object_result.stdout_text);

    const auto v1_put_invalid_schema =
        run_cli_in_dir(t81_bin,
                       {"canonfs", "put-file",
                        "examples/storage-and-canonfs/canonfs-interchange/v1/invalid-schema.json",
                        "--canonfs-root", "build/canonfs-v1-contract-root"},
                       repo_root);
    T81_TEST_CHECK(v1_put_invalid_schema.exit_code == 0);

    const auto v1_invalid_schema_result =
        run_cli_in_dir(t81_bin,
                       {"canonfs", "export", "sha3-256:1W÷r≈QiCFNI3Sμw×7JSKo≤71∞iEEbFVh4<MMTnKLb",
                        "--canonfs-root", "build/canonfs-v1-contract-root", "--out",
                        "build/canonfs-v1-contract-export/invalid-schema.bin", "--json"},
                       repo_root);
    T81_TEST_CHECK(v1_invalid_schema_result.exit_code != 0);
    T81_TEST_CHECK(read_file(v1_invalid_schema_output) == v1_invalid_schema_result.stdout_text);

    const auto v1_policy_denied_result = run_cli_in_dir(
        t81_bin,
        {"canonfs", "import", "examples/storage-and-canonfs/canonfs-interchange/v1/model.t81w",
         "--canonfs-root", "build/canonfs-v1-contract-root", "--policy",
         "examples/storage-and-canonfs/canonfs-interchange/v1/policy-deny-all.apl", "--json"},
        repo_root);
    T81_TEST_CHECK(v1_policy_denied_result.exit_code != 0);
    T81_TEST_CHECK(read_file(v1_policy_deny_output) == v1_policy_denied_result.stdout_text);

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

    const auto snapshot_diff_result =
        run_cli(t81_bin, {"canonfs", "snapshot-diff", snapshot_hash_a, snapshot_hash_b, "--json"});
    T81_TEST_CHECK(snapshot_diff_result.exit_code != 0);
    T81_TEST_CHECK(
        contains(snapshot_diff_result.stdout_text, "\"schema\": \"t81.canonfs-snapshot-diff.v1\""));

    const auto fsck_result = run_cli(t81_bin, {"canonfs", "fsck", "--json"});
    T81_TEST_CHECK(fsck_result.exit_code == 0);
    T81_TEST_CHECK(contains(fsck_result.stdout_text, "\"schema\": \"t81.canonfs-fsck.v1\""));

    const fs::path outside_payload = make_temp_path("t81-cli-contract-outside", ".txt");
    {
      std::ofstream out(outside_payload);
      out << "outside";
    }
    const fs::path symlink_root = make_temp_path("t81-cli-contract-canonfs-symlink", "");
    fs::create_directories(symlink_root, ignore_ec);
    T81_TEST_CHECK(!ignore_ec);
#if !defined(_WIN32)
    fs::create_symlink(outside_payload, symlink_root / "leak.txt", ignore_ec);
    if (!ignore_ec) {
      const auto symlink_snapshot_result = run_cli(
          t81_bin, {"canonfs", "snapshot", "--canonfs-root", symlink_root.string(), "--json"});
      T81_TEST_CHECK(symlink_snapshot_result.exit_code != 0);
      T81_TEST_CHECK(contains(symlink_snapshot_result.stdout_text, "symlinks are not permitted"));
    }
    ignore_ec.clear();
#endif
    const fs::path repair_root = make_temp_path("t81-cli-contract-canonfs-repair", "");
    const fs::path legacy_objects_dir = repair_root / "snapshots" / "legacy-snap" / "objects";
    fs::create_directories(legacy_objects_dir, ignore_ec);
    T81_TEST_CHECK(!ignore_ec);
    {
      std::ofstream out(legacy_objects_dir / "legacy.blk");
      out << "legacy-object-copy";
    }

    const auto repair_dry_run_result = run_cli(
        t81_bin,
        {"canonfs", "repair", "--canonfs-root", repair_root.string(), "--dry-run", "--json"});
    T81_TEST_CHECK(repair_dry_run_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(repair_dry_run_result.stdout_text, "\"schema\": \"t81.canonfs-repair.v1\""));
    T81_TEST_CHECK(contains(repair_dry_run_result.stdout_text, "\"dry_run\": true"));
    T81_TEST_CHECK(fs::exists(legacy_objects_dir));

    const auto repair_result =
        run_cli(t81_bin, {"canonfs", "repair", "--canonfs-root", repair_root.string(), "--json"});
    T81_TEST_CHECK(repair_result.exit_code == 0);
    T81_TEST_CHECK(contains(repair_result.stdout_text, "\"schema\": \"t81.canonfs-repair.v1\""));
    T81_TEST_CHECK(!fs::exists(legacy_objects_dir));

    const fs::path rollback_root = make_temp_path("t81-cli-contract-canonfs-rollback", "");
    fs::create_directories(rollback_root, ignore_ec);
    T81_TEST_CHECK(!ignore_ec);
    {
      std::ofstream out(rollback_root / "payload.txt");
      out << "rollback";
    }
    const auto rollback_snapshot_result = run_cli(
        t81_bin, {"canonfs", "snapshot", "--canonfs-root", rollback_root.string(), "--json"});
    T81_TEST_CHECK(rollback_snapshot_result.exit_code == 0);
#if !defined(_WIN32)
    const auto rollback_hash_pos = rollback_snapshot_result.stdout_text.find("\"hash\": \"");
    T81_TEST_CHECK(rollback_hash_pos != std::string::npos);
    const auto rollback_hash_start = rollback_hash_pos + std::string("\"hash\": \"").size();
    const auto rollback_hash_end =
        rollback_snapshot_result.stdout_text.find('"', rollback_hash_start);
    T81_TEST_CHECK(rollback_hash_end != std::string::npos);
    const std::string rollback_hash = rollback_snapshot_result.stdout_text.substr(
        rollback_hash_start, rollback_hash_end - rollback_hash_start);
    fs::create_symlink(outside_payload, rollback_root / "snapshots" / rollback_hash / "leak.txt",
                       ignore_ec);
    if (!ignore_ec) {
      const auto rollback_result =
          run_cli(t81_bin, {"canonfs", "rollback", "--canonfs-root", rollback_root.string(), "--to",
                            rollback_hash, "--json"});
      T81_TEST_CHECK(rollback_result.exit_code != 0);
      T81_TEST_CHECK(contains(rollback_result.stdout_text, "symlinks are not permitted"));
    }
    ignore_ec.clear();
#endif
    fs::remove(payload, ignore_ec);
    fs::remove(canonfs_policy, ignore_ec);
    fs::remove(invalid_schema_object, ignore_ec);
    fs::remove(tampered_source_two, ignore_ec);
    fs::remove(malformed_manifest, ignore_ec);
    fs::remove(imported_file_restore, ignore_ec);
    fs::remove(restored, ignore_ec);
    fs::remove(payload_two, ignore_ec);
    fs::remove(outside_payload, ignore_ec);
    fs::remove_all(import_tree, ignore_ec);
    fs::remove_all(export_tree, ignore_ec);
    fs::remove_all(malformed_export_target, ignore_ec);
    fs::remove_all(tampered_root, ignore_ec);
    fs::remove_all(symlink_root, ignore_ec);
    fs::remove_all(repair_root, ignore_ec);
    fs::remove_all(rollback_root, ignore_ec);
  }

  {
    const fs::path canonfs_root = make_temp_path("t81-cli-contract-model-canonfs", "");
    std::error_code ignore_ec;
    fs::create_directories(canonfs_root, ignore_ec);
    T81_TEST_CHECK(!ignore_ec);

    const fs::path model_path = make_temp_path("t81-cli-contract-model", ".t81w");
    t81::weights::NativeModel model;

    t81::weights::NativeTensor mat_a;
    mat_a.shape = {2, 2};
    mat_a.trits = 4;
    mat_a.data = {40};
    model["mat_a"] = mat_a;

    t81::weights::NativeTensor mat_b;
    mat_b.shape = {2, 2};
    mat_b.trits = 4;
    mat_b.data = {67};
    model["mat_b"] = mat_b;

    t81::weights::save_t81w(model, model_path);
    const auto loaded_model = t81::weights::load_t81w(model_path);
    T81_TEST_CHECK(!loaded_model.checksum.empty());

    const auto put_model_result = run_cli(t81_bin, {"canonfs", "put-file", model_path.string(),
                                                    "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(put_model_result.exit_code == 0);
    T81_TEST_CHECK(contains(put_model_result.stdout_text, "sha3-256:"));
    std::string model_hash = put_model_result.stdout_text;
    while (!model_hash.empty() && std::isspace(static_cast<unsigned char>(model_hash.back()))) {
      model_hash.pop_back();
    }

    const fs::path tensor_fixture = fs::absolute(t81_bin).parent_path().parent_path() / "tests" /
                                    "fixtures" / "t81lang_std_tensor" / "03_matmul_weights.t81";
    T81_TEST_CHECK(fs::exists(tensor_fixture));
    const auto missing_weights_run_result =
        run_cli(t81_bin, {"code", "run", tensor_fixture.string()});
    const fs::path allow_policy = make_temp_path("t81-cli-contract-model-allow", ".apl");
    const fs::path deny_policy = make_temp_path("t81-cli-contract-model-deny", ".apl");

    {
      std::ofstream out(allow_policy);
      T81_TEST_CHECK(static_cast<bool>(out));
      out << "(policy\n"
             "  (tier 2)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:"
          << loaded_model.checksum << "\"]))\n";
    }
    {
      std::ofstream out(deny_policy);
      T81_TEST_CHECK(static_cast<bool>(out));
      out << "(policy\n"
             "  (tier 2)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:cafebabe\"]))\n";
    }

    set_env("T81_CANONFS_ROOT", canonfs_root.string());
    const auto model_run_result =
        run_cli(t81_bin, {"code", "run", tensor_fixture.string(), "--weights-model", model_hash});
    const auto allowed_model_run_result =
        run_cli(t81_bin, {"code", "run", tensor_fixture.string(), "--weights-model", model_hash,
                          "--policy", allow_policy.string()});
    const auto denied_model_run_result =
        run_cli(t81_bin, {"code", "run", tensor_fixture.string(), "--weights-model", model_hash,
                          "--policy", deny_policy.string()});
    unset_env("T81_CANONFS_ROOT");

    T81_TEST_CHECK(missing_weights_run_result.exit_code != 0);
    T81_TEST_CHECK(
        appears_in_order(missing_weights_run_result.stderr_text,
                         {"Compilation successful → ", "error: Execution trapped: DecodeFault",
                          "hint: tensor-backed programs often require --weights-model"}));
    T81_TEST_CHECK(model_run_result.exit_code == 0);
    T81_TEST_CHECK(contains(model_run_result.stdout_text, "<tensor#1>"));
    T81_TEST_CHECK(contains(model_run_result.stderr_text, "result kind: tensor_handle"));
    T81_TEST_CHECK(contains(model_run_result.stderr_text,
                            "operation: tensor-backed execution completed under attached weights"));
    T81_TEST_CHECK(
        contains(model_run_result.stderr_text,
                 "next (progress): none on this path; the result remains a runtime tensor handle"));
    T81_TEST_CHECK(contains(
        model_run_result.stderr_text,
        "next (inspect): rerun with --trace to inspect the executed tensor ops and Axion events"));
    T81_TEST_CHECK(allowed_model_run_result.exit_code == 0);
    T81_TEST_CHECK(contains(allowed_model_run_result.stdout_text, "<tensor#1>"));
    T81_TEST_CHECK(denied_model_run_result.exit_code != 0);
    T81_TEST_CHECK(contains(denied_model_run_result.stderr_text, "SecurityFault"));
    T81_TEST_CHECK(
        appears_in_order(denied_model_run_result.stderr_text,
                         {"Compilation successful → ", "error: Execution trapped: SecurityFault",
                          "reason: ", "opcode: TMatMul", "compute executed: no"}));
    T81_TEST_CHECK(!contains(denied_model_run_result.stdout_text, "<tensor#1>"));

    fs::remove(model_path, ignore_ec);
    fs::remove(allow_policy, ignore_ec);
    fs::remove(deny_policy, ignore_ec);
    fs::remove_all(canonfs_root, ignore_ec);
  }

  {
    const fs::path model_path = make_temp_path("t81-cli-ai-inference-model", ".t81w");
    t81::weights::NativeModel model;

    t81::weights::NativeTensor q_proj;
    q_proj.shape = {2, 2};
    q_proj.trits = 4;
    q_proj.data = {40};
    model["model.layers.0.self_attn.q_proj.weight"] = q_proj;

    t81::weights::NativeTensor k_proj;
    k_proj.shape = {2, 2};
    k_proj.trits = 4;
    k_proj.data = {67};
    model["model.layers.0.self_attn.k_proj.weight"] = k_proj;

    t81::weights::save_t81w(model, model_path);

    const auto ai_result = run_cli(
        t81_bin, {"ai", "inference", "run", "--model", "contract-demo", "--model-file",
                  model_path.string(), "--mode", "strict_deterministic", "--prompt", "hello"});

    T81_TEST_CHECK(ai_result.exit_code == 0);
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"schema\": \"t81.ai.inference-run.v1\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"selected_backend\": \"t81_reference_vm\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"execution_kind\": \"real_vm_native_probe\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"architecture_profile\": \"unknown\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"probe_kind\": \"llama_qk_projection\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"has_config_json\": false"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"has_tokenizer_json\": false"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"readiness\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"artifact_visibility\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"kind\": \"ready\""));
    T81_TEST_CHECK(
        contains(ai_result.stdout_text,
                 "\"candidate_selection_evidence_policy\": \"not_applicable_single_probe.v1\""));
    T81_TEST_CHECK(
        contains(ai_result.stdout_text, "\"output_policy\": \"verbatim_native_probe.v1\""));
    T81_TEST_CHECK(
        contains(ai_result.stdout_text, "\"lhs\": \"model.layers.0.self_attn.q_proj.weight\""));
    T81_TEST_CHECK(
        contains(ai_result.stdout_text, "\"rhs\": \"model.layers.0.self_attn.k_proj.weight\""));
    T81_TEST_CHECK(appears_in_order(
        ai_result.stdout_text,
        {"\"result_summary\": ", "\"result_class\": \"tensor_handle\"", "\"result_meaning\": ",
         "\"next_step_hint\": ", "\"termination_reason\": \"no_logits_row_probe\"",
         "\"output_kind\": \"tensor_handle\"", "\"output_preview\": \"<tensor#1>\"",
         "\"backend_selection_trace_sha256\": "}));
    T81_TEST_CHECK(contains(ai_result.stdout_text,
                            "\"next_step_hint\": \"next (progress): none on this path; this run "
                            "does not expose a richer bounded decode result. next (inspect): "
                            "inspect output_summary for handle semantics\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"output\": \"<tensor#1>\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"generated_tokens\": 1"));

    std::error_code ignore_ec;
    fs::remove(model_path, ignore_ec);
  }

  {
    const fs::path model_dir = make_temp_path("t81-cli-ai-inference-llama-model-dir", "");
    fs::create_directories(model_dir);
    const fs::path model_path = model_dir / "contract-llama-demo.t81w";
    const fs::path tokenizer_path = model_dir / "tokenizer.json";
    t81::weights::NativeModel model;

    const auto make_tensor = [](std::initializer_list<std::uint32_t> shape) {
      t81::weights::NativeTensor tensor;
      tensor.shape.assign(shape.begin(), shape.end());
      std::size_t element_count = 1;
      for (std::uint32_t dim : tensor.shape) {
        element_count *= static_cast<std::size_t>(dim);
      }
      tensor.trits = static_cast<std::uint64_t>(element_count);
      tensor.data.assign((element_count + 47u) / 48u, 0u);
      return tensor;
    };

    model["model.embed_tokens.weight"] = make_tensor({32, 16});
    model["model.norm.weight"] = make_tensor({16});
    model["model.layers.0.self_attn.q_proj.weight"] = make_tensor({16, 16});
    model["model.layers.0.self_attn.k_proj.weight"] = make_tensor({16, 16});
    model["model.layers.0.self_attn.v_proj.weight"] = make_tensor({16, 16});
    model["model.layers.0.self_attn.o_proj.weight"] = make_tensor({16, 16});
    model["model.layers.0.mlp.gate_proj.weight"] = make_tensor({16, 16});
    model["model.layers.0.mlp.up_proj.weight"] = make_tensor({16, 16});
    model["model.layers.0.mlp.down_proj.weight"] = make_tensor({16, 16});
    model["model.layers.1.self_attn.q_proj.weight"] = make_tensor({16, 16});
    model["model.layers.1.self_attn.k_proj.weight"] = make_tensor({16, 16});
    model["model.layers.1.self_attn.v_proj.weight"] = make_tensor({16, 16});
    model["lm_head.weight"] = make_tensor({64, 16});

    t81::weights::save_t81w(model, model_path);
    {
      std::ofstream out(tokenizer_path);
      out << R"({
  "model": {
    "type": "BPE",
    "vocab": {
      "greet": 7,
      "hello": 11,
      "world": 12,
      "▁greet": 17,
      "▁hello": 21
    }
  }
}
)";
    }

    const auto ai_result =
        run_cli(t81_bin, {"ai", "inference", "run", "--model", "contract-llama-demo",
                          "--model-file", model_path.string(), "--mode", "strict_deterministic",
                          "--prompt", "greet hello", "--max-tokens", "2"});

    T81_TEST_CHECK(ai_result.exit_code == 0);
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"architecture_profile\": \"llama-dense-v1\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text,
                            "\"probe_kind\": \"llama_two_layer_attention_row_logits_sample\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"logits_row_probe_supported\": true"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"logits_sample_window\": 8"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"mode\": \"tokenizer_prompt_history\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"tokenizer_seed_supported\": true"));
    T81_TEST_CHECK(contains(ai_result.stdout_text,
                            "\"basis\": \"tokenizer_prompt_history_contiguous_window.v1\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"evidence_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"vocab_size\": 64"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"prompt_token_history_count\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"seed_token_id\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"window_start\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"window_end\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"candidate_window_count\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"candidate_window_signature_sha256\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"logits_evidence_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"sampled_logits_count\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"sampled_logits_signature_sha256\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"selected_candidate\": {\"token_id\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"stateful_decode_supported\": false"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"requested_max_tokens\": 2"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"stability_recovery_exhausted\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"recovery_triggered\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"recovery_steps\": ["));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"weak_steps\": ["));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"bounded_decode_health\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"readiness\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text,
                            "\"confidence_envelope\": \"bounded_architecture_state_probe.v1\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"decode_trace_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"decode_trace_truncated\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"decode_trace_detail_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"termination_reason\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"decode_trace\": ["));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"step\": 0"));
    T81_TEST_CHECK(
        contains(ai_result.stdout_text, "\"state_kind\": \"prompt_history_bounded_context.v1\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"transition_kind\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text,
                            "\"window_selection_kind\": "
                            "\"hidden_state_class_projection_mode_conditioned_window_seed.v1\""));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"candidate_basis_kind\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"sample_window_kind\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"selection_policy_kind\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"decode_mode_kind\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"next_window_start\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"sample_window_used\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"context_window_used\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"stability\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"final_decode_state\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"degraded_artifact_summary\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"artifact_visibility\": {"));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"candidate_selection_evidence_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"logits_evidence_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"decode_trace_detail_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"output_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"output_summary\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"generated_token_ids\": ["));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"generated_preview_policy\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"generated_token_pieces\": ["));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"generated_text_preview\": "));
    T81_TEST_CHECK(contains(ai_result.stdout_text, "\"status\": "));

    const fs::path forward_state_model_path = model_dir / "contract-forward-state-demo.t81w";
    t81::weights::NativeModel forward_state_model;
    forward_state_model["model.embed_tokens.weight"] = make_tensor({64, 16});
    forward_state_model["model.norm.weight"] = make_tensor({16});
    forward_state_model["model.layers.0.self_attn.q_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.0.self_attn.k_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.0.self_attn.v_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.0.self_attn.o_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.0.mlp.gate_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.0.mlp.up_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.0.mlp.down_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.1.self_attn.q_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.1.self_attn.k_proj.weight"] = make_tensor({16, 16});
    forward_state_model["model.layers.1.self_attn.v_proj.weight"] = make_tensor({16, 16});
    forward_state_model["lm_head.weight"] = make_tensor({64, 16});
    t81::weights::save_t81w(forward_state_model, forward_state_model_path);

    const auto bounded_ready_ai_result =
        run_cli(t81_bin, {"ai", "inference", "run", "--model", "contract-forward-state-demo",
                          "--model-file", forward_state_model_path.string(), "--mode",
                          "strict_deterministic", "--prompt", "greet_hello", "--max-tokens", "1"});

    T81_TEST_CHECK(bounded_ready_ai_result.exit_code == 0);
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"bounded_decode_health\": {"));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"kind\": \"healthy\""));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"readiness\": {"));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"kind\": \"ready\""));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"recovery_triggered\": false"));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"recovery_steps\": []"));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"weak_steps\": []"));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"artifact_visibility\": {"));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text,
                            "\"confidence_envelope\": \"bounded_architecture_state_probe.v1\""));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"kind\": \"ready\""));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"forward_state_kind\": "));
    T81_TEST_CHECK(
        contains(bounded_ready_ai_result.stdout_text, "\"forward_state_signature_sha256\": "));
    T81_TEST_CHECK(
        contains(bounded_ready_ai_result.stdout_text, "\"forward_state_generation\": 0"));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text, "\"forward_state_class\": "));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text,
                            "\"forward_state_class_signature_sha256\": "));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text,
                            "\"output_policy\": \"verbatim_native_probe.v1\""));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text,
                            "\"generated_preview_policy\": \"full_sequence.v1\""));
    T81_TEST_CHECK(appears_in_order(
        bounded_ready_ai_result.stdout_text,
        {"\"result_summary\": ", "\"result_class\": \"bounded_inference_text\"",
         "\"result_meaning\": ", "\"next_step_hint\": ",
         "\"termination_reason\": \"max_tokens_reached\"", "\"output_kind\": \"raw_vm_probe_text\"",
         "\"output_preview\": ", "\"backend_selection_trace_sha256\": "}));
    T81_TEST_CHECK(contains(bounded_ready_ai_result.stdout_text,
                            "\"termination_reason\": \"max_tokens_reached\""));

    const auto forward_state_ai_result =
        run_cli(t81_bin, {"ai", "inference", "run", "--model", "contract-forward-state-demo",
                          "--model-file", forward_state_model_path.string(), "--mode",
                          "strict_deterministic", "--prompt", "greet_hello", "--max-tokens", "2"});

    T81_TEST_CHECK(forward_state_ai_result.exit_code == 0);
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"readiness\": {"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"bounded_decode_health\": {"));
    T81_TEST_CHECK(appears_in_order(
        forward_state_ai_result.stdout_text,
        {"\"result_summary\": ", "\"result_class\": \"bounded_inference_text\"",
         "\"result_meaning\": ", "\"next_step_hint\": ",
         "\"termination_reason\": \"max_tokens_reached\"", "\"output_kind\": \"raw_vm_probe_text\"",
         "\"output_preview\": ", "\"backend_selection_trace_sha256\": "}));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text,
                 "\"next_step_hint\": \"next (progress): none on this path; this run does not "
                 "expose a richer user-visible result beyond the tokenizer-only preview. next "
                 "(inspect): inspect generated_preview_summary, then forward_state_summary or "
                 "decode_trace for deeper evidence\""));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"true_state_carry_supported\": true"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"state_carry_limitations\": {"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"kind\": \"bounded_intermediate_tensor_literal_import.v1\""));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"intermediate_tensor_import_supported\": true"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"import_path\": \"compiled_tensor_literal_reimport.v1\""));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"intermediate_tensor_export_supported\": true"));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"intermediate_tensor_import_used\": true"));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"intermediate_tensor_blend_used\": true"));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text,
                 "\"hidden_tensor_carry_mode_kind\": \"evolved_hidden_tensor_feedback.v1\""));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"hidden_tensor_summary\": {"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"kind\": \"vm_intermediate_tensor_export.v1\""));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"hidden_tensor_signature_sha256\": \""));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"generated_tokens\": "));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"termination_reason\": "));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"recovery_steps\": ["));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"weak_steps\": ["));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"decode_trace_detail_policy\": "));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"artifact_visibility\": {"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"status\": "));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"forward_state_summary\": {"));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"max_consumed_forward_state_count\": "));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"state_input_summary\": {"));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"max_state_input_row_count\": "));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"kv_state_summary\": {"));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"kind\": \"bounded_qk_tensor_state.v1\""));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"kv_state_signature_sha256\": \""));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"architecture_state_summary\": {"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"kind\": \"bounded_hidden_tensor_qk_forward_state.v1\""));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"architecture_state_signature_sha256\": \""));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"forward_state_kind\": \"projection_carried_forward_state.v1\""));
    T81_TEST_CHECK(
        contains(forward_state_ai_result.stdout_text, "\"forward_state_generation\": 1"));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text, "\"forward_state_class\": "));
    T81_TEST_CHECK(contains(forward_state_ai_result.stdout_text,
                            "\"transition_kind\": \"forward_state_feedback_state_transition.v1\""));

    const auto forward_state_three_step_result = run_cli_with_timeout(
        t81_bin,
        {"ai", "inference", "run", "--model", "contract-forward-state-demo", "--model-file",
         forward_state_model_path.string(), "--mode", "strict_deterministic", "--prompt",
         "greet_hello", "--max-tokens", "3"},
        std::chrono::seconds(60));

    T81_TEST_CHECK(forward_state_three_step_result.exit_code == 0);
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text, "\"generated_tokens\": "));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"forward_state_summary\": {"));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"state_input_summary\": {"));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"kv_state_summary\": {"));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"architecture_state_summary\": {"));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"kind\": \"bounded_qk_tensor_state.v1\""));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text,
                 "\"kv_state_carry_mode_kind\": \"architecture_state_evolved_qk_signature.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"kind\": \"bounded_hidden_tensor_qk_forward_state.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"kind\": \"bounded_intermediate_tensor_literal_import.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"intermediate_tensor_export_supported\": true"));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"intermediate_tensor_import_used\": true"));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"intermediate_tensor_blend_used\": true"));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"hidden_tensor_carry_mode_kind\": "
                            "\"architecture_state_evolved_hidden_tensor_feedback.v1\""));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"hidden_tensor_summary\": {"));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"kv_state_signature_sha256\": \""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"final_hidden_tensor_signature_sha256\": \""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text, "\"feedback_steps\": "));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"final_kv_state_signature_sha256\": \""));
    T81_TEST_CHECK(contains(
        forward_state_three_step_result.stdout_text,
        "\"final_kv_state_carry_mode_kind\": \"architecture_state_evolved_qk_signature.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text, "\"feedback_steps\": "));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"final_architecture_state_signature_sha256\": \""));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"architecture_state_class\": \""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text, "\"feedback_steps\": "));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"architecture_state_confidence_score\": "));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"architecture_state_stability_kind\": \""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"confidence_envelope\": \"bounded_architecture_state_probe.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"architecture_state_guardrail_triggered\": "));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"artifact_visibility\": {"));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"max_confidence_score\": "));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text, "\"final_stability_kind\": \""));
    T81_TEST_CHECK(
        contains(forward_state_three_step_result.stdout_text,
                 "\"transition_kind\": \"architecture_state_feedback_state_transition.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"candidate_basis_kind\": \"architecture_state_feedback_window.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"decode_mode_kind\": \"architecture_state_feedback_projection.v1\""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"final_state_input_signature_sha256\": \""));
    T81_TEST_CHECK(contains(forward_state_three_step_result.stdout_text,
                            "\"transition_kind\": \"forward_state_feedback_state_transition.v1\""));

    const auto forward_state_four_step_result = run_cli_with_timeout(
        t81_bin,
        {"ai", "inference", "run", "--model", "contract-forward-state-demo", "--model-file",
         forward_state_model_path.string(), "--mode", "strict_deterministic", "--prompt",
         "greet_hello", "--max-tokens", "4"},
        std::chrono::seconds(60));

    T81_TEST_CHECK(forward_state_four_step_result.exit_code == 0);
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text, "\"generated_tokens\": 4"));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"decode_trace_full_steps\": 4"));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"bounded_horizon_steps\": 4"));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"bounded_horizon_remaining\": 0"));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"bounded_horizon_utilization\": 1"));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"bounded_horizon_reached\": true"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"termination_reason\": "
                            "\"deep_architecture_state_horizon_reached\""));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"hidden_tensor_summary\": {"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text, "\"feedback_steps\": 3"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text, "\"kv_state_summary\": {"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text, "\"feedback_steps\": 4"));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"architecture_state_summary\": {"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"confidence_envelope\": "
                            "\"bounded_deep_architecture_state_probe.v1\""));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"deep_feedback_used\": true"));
    T81_TEST_CHECK(
        contains(forward_state_four_step_result.stdout_text, "\"deep_feedback_steps\": 1"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text, "\"utilization\": 0.5"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"architecture_state_deep_feedback_used\": true"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text, "\"feedback_steps\": 2"));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"transition_kind\": "
                            "\"architecture_state_feedback_state_transition.v1\""));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"transition_kind\": "
                            "\"architecture_state_deep_feedback_state_transition.v1\""));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"candidate_basis_kind\": \"architecture_state_feedback_window.v1\""));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"candidate_basis_kind\": "
                            "\"architecture_state_deep_feedback_window.v1\""));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"decode_mode_kind\": "
                            "\"architecture_state_feedback_projection.v1\""));
    T81_TEST_CHECK(contains(forward_state_four_step_result.stdout_text,
                            "\"decode_mode_kind\": "
                            "\"architecture_state_deep_feedback_projection.v1\""));

    const fs::path degraded_model_path = model_dir / "contract-degraded-demo.t81w";
    const fs::path degraded_tokenizer_path = model_dir / "tokenizer.json";
    t81::weights::NativeModel degraded_model;
    degraded_model["model.embed_tokens.weight"] = make_tensor({16, 16});
    degraded_model["model.norm.weight"] = make_tensor({16});
    degraded_model["model.layers.0.self_attn.q_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.0.self_attn.k_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.0.self_attn.v_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.0.self_attn.o_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.0.mlp.gate_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.0.mlp.up_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.0.mlp.down_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.1.self_attn.q_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.1.self_attn.k_proj.weight"] = make_tensor({16, 16});
    degraded_model["model.layers.1.self_attn.v_proj.weight"] = make_tensor({16, 16});
    degraded_model["lm_head.weight"] = make_tensor({64, 16});
    t81::weights::save_t81w(degraded_model, degraded_model_path);
    {
      std::ofstream out(degraded_tokenizer_path);
      out << R"({
  "model": {
    "type": "BPE",
    "vocab": {
      "greet": 7,
      "hello": 11,
      "world": 12,
      "▁greet": 17,
      "▁hello": 21
    }
  }
}
)";
    }

    const auto degraded_ai_result =
        run_cli(t81_bin, {"ai", "inference", "run", "--model", "contract-degraded-demo",
                          "--model-file", degraded_model_path.string(), "--mode",
                          "strict_deterministic", "--prompt", "greet hello", "--max-tokens", "2"});

    T81_TEST_CHECK(degraded_ai_result.exit_code == 0);
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text, "\"bounded_decode_health\": {"));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text, "\"kind\": \"degraded\""));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text, "\"readiness\": {"));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text, "\"kind\": \"degraded\""));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text,
                            "\"termination_reason\": \"decode_probe_unavailable\""));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text, "\"artifact_visibility\": {"));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text, "\"kind\": \"degraded\""));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text,
                            "\"output_policy\": \"suppressed_on_degraded.v1\""));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text,
                            "\"generated_preview_policy\": \"first_token_only_on_degraded.v1\""));
    T81_TEST_CHECK(contains(degraded_ai_result.stdout_text, "\"degraded_artifact_summary\": {"));

    std::error_code ignore_ec;
    fs::remove_all(model_dir, ignore_ec);
  }

  {
    const auto artifact_help = run_cli(t81_bin, {"help", "artifact"});
    T81_TEST_CHECK(artifact_help.exit_code == 0);
    T81_TEST_CHECK(
        contains(artifact_help.stdout_text, "t81.ai.task.assess-fixed.host-action-record.v1"));
    T81_TEST_CHECK(contains(artifact_help.stdout_text, "t81.ai.task.assess-fixed.bundle.v1"));
    T81_TEST_CHECK(
        contains(artifact_help.stdout_text, "t81.ai.task.route-fixed.path-selection-record.v1"));
    T81_TEST_CHECK(contains(artifact_help.stdout_text, "t81.ai.task.route-fixed.bundle.v1"));
    T81_TEST_CHECK(
        contains(artifact_help.stdout_text, "t81.ai.task.classify-fixed.rule-selection-record.v1"));
    T81_TEST_CHECK(contains(artifact_help.stdout_text, "t81.ai.task.classify-fixed.bundle.v1"));
    T81_TEST_CHECK(contains(artifact_help.stdout_text, "Canonical runnable example:"));
    T81_TEST_CHECK(contains(artifact_help.stdout_text, "run_assess_fixed_host_action.sh"));
    T81_TEST_CHECK(contains(artifact_help.stdout_text, "run_route_fixed_path_selection.sh"));
    T81_TEST_CHECK(contains(artifact_help.stdout_text, "run_classify_fixed_rule_selection.sh"));
  }

  {
    const fs::path model_dir = make_temp_path("t81-cli-answer-fixed-model-dir", "");
    write_answer_fixed_demo_model(model_dir);
    const fs::path model_path = model_dir / "answer-fixed-demo.t81w";
    const fs::path canonfs_root = make_temp_path("t81-cli-answer-fixed-canonfs", "");
    fs::create_directories(canonfs_root);

    const auto hash_result = run_cli(t81_bin, {"determinism", "hash", model_path.string()});
    T81_TEST_CHECK(hash_result.exit_code == 0);
    std::istringstream hash_stream(hash_result.stdout_text);
    std::string raw_model_hash;
    hash_stream >> raw_model_hash;
    T81_TEST_CHECK(!raw_model_hash.empty());

    const fs::path allow_policy = make_temp_path("t81-cli-answer-fixed-allow", ".apl");
    {
      std::ofstream out(allow_policy);
      out << "(policy\n"
             "  (tier 1)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:"
          << raw_model_hash
          << "\"])\n"
             "  (require-axion-event (reason \"task:answer_fixed.v1\")))\n";
    }

    const fs::path deny_policy = make_temp_path("t81-cli-answer-fixed-deny", ".apl");
    {
      std::ofstream out(deny_policy);
      out << "(policy\n"
             "  (tier 1)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:"
          << raw_model_hash << "\"]))\n";
    }

    const auto allow_result =
        run_cli(t81_bin, {"ai", "task", "answer-fixed", "--model", "answer-fixed-demo",
                          "--model-file", model_path.string(), "--policy", allow_policy.string(),
                          "--canonfs-root", canonfs_root.string(), "--mode", "strict_deterministic",
                          "--input", "greet hello"});
    T81_TEST_CHECK(allow_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(allow_result.stdout_text, "\"schema\": \"t81.ai.task-run.answer-fixed.v1\""));
    T81_TEST_CHECK(appears_in_order(
        allow_result.stdout_text,
        {"\"result_summary\": ", "\"result_class\": \"ai_task_result\"", "\"result_meaning\": ",
         "\"next_step_hint\": ", "\"termination_reason\": \"single_step_max_score\"",
         "\"output_kind\": \"canonical_task_answer\"", "\"output_preview\": \"YES\"",
         "\"result_ref\": ", "\"provenance_ref\": ", "\"policy_result\": \"allowed\""}));
    T81_TEST_CHECK(contains(allow_result.stdout_text, "\"answer\": \"YES\""));
    T81_TEST_CHECK(contains(allow_result.stdout_text, "\"answer_token_ids\": [42]"));

    const auto result_ref = extract_json_string(allow_result.stdout_text, "result_ref");
    const auto provenance_ref = extract_json_string(allow_result.stdout_text, "provenance_ref");
    T81_TEST_CHECK(result_ref.has_value());
    T81_TEST_CHECK(provenance_ref.has_value());

    const auto result_artifact =
        run_cli(t81_bin, {"canonfs", "get", *result_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(result_artifact.exit_code == 0);
    T81_TEST_CHECK(
        contains(result_artifact.stdout_text, "\"schema\": \"t81.ai.task.answer-fixed.v1\""));
    T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"answer\": \"YES\""));
    T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"answer_token_ids\": [42]"));

    const auto provenance_artifact = run_cli(
        t81_bin, {"canonfs", "get", *provenance_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(provenance_artifact.exit_code == 0);
    T81_TEST_CHECK(
        contains(provenance_artifact.stdout_text, "\"schema\": \"t81.ai.task.provenance.v1\""));
    T81_TEST_CHECK(contains(provenance_artifact.stdout_text, "\"policy_result\": \"allowed\""));
    T81_TEST_CHECK(contains(provenance_artifact.stdout_text, "\"selected_token_id\": 42"));

    const auto deny_result =
        run_cli(t81_bin, {"ai", "task", "answer-fixed", "--model", "answer-fixed-demo",
                          "--model-file", model_path.string(), "--policy", deny_policy.string(),
                          "--canonfs-root", canonfs_root.string(), "--mode", "strict_deterministic",
                          "--input", "greet hello"});
    T81_TEST_CHECK(deny_result.exit_code == 13);
    T81_TEST_CHECK(contains(deny_result.stderr_text, "SecurityFault"));
    T81_TEST_CHECK(contains(deny_result.stderr_text,
                            "answer-fixed denied (policy is missing require-axion-event reason"));

    std::error_code ignore_ec;
    fs::remove(allow_policy, ignore_ec);
    fs::remove(deny_policy, ignore_ec);
    fs::remove_all(model_dir, ignore_ec);
    fs::remove_all(canonfs_root, ignore_ec);
  }

  {
    const fs::path model_dir = make_temp_path("t81-cli-assess-fixed-model-dir", "");
    const fs::path canonfs_root = make_temp_path("t81-cli-assess-fixed-canonfs", "");
    const fs::path action_dir = make_temp_path("t81-cli-assess-fixed-actions", "");
    fs::create_directories(canonfs_root);
    fs::create_directories(action_dir);

    const fs::path builder = fs::absolute(t81_bin).parent_path() / "t81_make_assess_fixed_demo";
    T81_TEST_CHECK(fs::exists(builder));
    const auto build_result = run_cli(builder, {model_dir.string()});
    T81_TEST_CHECK(build_result.exit_code == 0);

    const fs::path model_path = model_dir / "assess-fixed-demo.t81w";
    const auto hash_result = run_cli(t81_bin, {"determinism", "hash", model_path.string()});
    T81_TEST_CHECK(hash_result.exit_code == 0);
    std::istringstream hash_stream(hash_result.stdout_text);
    std::string raw_model_hash;
    hash_stream >> raw_model_hash;
    T81_TEST_CHECK(!raw_model_hash.empty());

    const fs::path allow_policy = make_temp_path("t81-cli-assess-fixed-allow", ".apl");
    {
      std::ofstream out(allow_policy);
      out << "(policy\n"
             "  (tier 1)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:"
          << raw_model_hash
          << "\"])\n"
             "  (require-axion-event (reason \"task:assess_fixed.v1\")))\n";
    }

    const auto task_result =
        run_cli(t81_bin, {"ai", "task", "assess-fixed", "--model", "assess-fixed-demo",
                          "--model-file", model_path.string(), "--policy", allow_policy.string(),
                          "--canonfs-root", canonfs_root.string(), "--mode", "strict_deterministic",
                          "--input", "greet hello"});
    T81_TEST_CHECK(task_result.exit_code == 0);
    const auto result_ref = extract_json_string(task_result.stdout_text, "result_ref");
    const auto provenance_ref = extract_json_string(task_result.stdout_text, "provenance_ref");
    T81_TEST_CHECK(result_ref.has_value());
    T81_TEST_CHECK(provenance_ref.has_value());

    const fs::path action_path = action_dir / "allow.marker";
    {
      std::ofstream out(action_path);
      out << "decision=ALLOW\n"
          << "reason_code=GREETING_PAIR\n"
          << "source_result_ref=" << *result_ref << "\n";
    }
    const auto action_ref_result = run_cli(t81_bin, {"canonfs", "put-file", action_path.string(),
                                                     "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(action_ref_result.exit_code == 0);
    std::string action_ref = action_ref_result.stdout_text;
    while (!action_ref.empty() && (action_ref.back() == '\n' || action_ref.back() == '\r')) {
      action_ref.pop_back();
    }
    T81_TEST_CHECK(!action_ref.empty());

    const auto write_store_result =
        run_cli(t81_bin, {"artifact",       "write-store-record",
                          "--schema",       "t81.ai.task.assess-fixed.host-action-record.v1",
                          "--field",        "source_result_ref=" + *result_ref,
                          "--field",        "source_provenance_ref=" + *provenance_ref,
                          "--field",        "decision=ALLOW",
                          "--field",        "reason_code=GREETING_PAIR",
                          "--field",        "termination_reason=single_step_max_score",
                          "--field",        "selected_action=write_allow_marker",
                          "--field",        "selected_path=actions/allow.marker",
                          "--field",        "action_ref=" + action_ref,
                          "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(write_store_result.exit_code == 0);
    T81_TEST_CHECK(contains(write_store_result.stdout_text,
                            "\"schema\": \"t81.artifact.record-write-store.v1\""));
    T81_TEST_CHECK(appears_in_order(
        write_store_result.stdout_text,
        {"\"result_summary\": ", "\"schema_id\": ", "\"validation_result\": \"pass\"",
         "\"record_ref\": ", "\"next_step_hint\": ", "\"status\": \"pass\""}));
    const auto record_ref = extract_json_string(write_store_result.stdout_text, "record_ref");
    T81_TEST_CHECK(record_ref.has_value());

    const auto bundle_store_result = run_cli(
        t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.assess-fixed.bundle.v1",
                  "--field", "source_result_ref=" + *result_ref, "--field",
                  "source_provenance_ref=" + *provenance_ref, "--field", "action_ref=" + action_ref,
                  "--field", "record_ref=" + *record_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(bundle_store_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(bundle_store_result.stdout_text, "\"schema\": \"t81.artifact.bundle-store.v1\""));
    T81_TEST_CHECK(appears_in_order(
        bundle_store_result.stdout_text,
        {"\"result_summary\": ", "\"schema_id\": ", "\"validation_result\": \"pass\"",
         "\"bundle_ref\": ", "\"next_step_hint\": ", "\"status\": \"pass\""}));
    const auto bundle_ref = extract_json_string(bundle_store_result.stdout_text, "bundle_ref");
    T81_TEST_CHECK(bundle_ref.has_value());

    const auto bundle_artifact =
        run_cli(t81_bin, {"canonfs", "get", *bundle_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(bundle_artifact.exit_code == 0);
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text,
                            "\"schema\": \"t81.ai.task.assess-fixed.bundle.v1\""));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"source_result_ref\": "));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"source_provenance_ref\": "));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"action_ref\": "));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"record_ref\": "));

    std::error_code ignore_ec;
    fs::remove(allow_policy, ignore_ec);
    fs::remove_all(model_dir, ignore_ec);
    fs::remove_all(canonfs_root, ignore_ec);
    fs::remove_all(action_dir, ignore_ec);
  }

  {
    const fs::path hello_world = fs::absolute(t81_bin).parent_path().parent_path() / "examples" /
                                 "core-language" / "hello_world.t81";
    const auto memory_stats_result =
        run_cli(t81_bin, {"internal", "memory-stats", hello_world.string()});
    T81_TEST_CHECK(memory_stats_result.exit_code == 0);
    T81_TEST_CHECK(contains(memory_stats_result.stdout_text, "Memory Pool Analysis"));
    T81_TEST_CHECK(contains(memory_stats_result.stdout_text, "Stack peak usage"));
  }

  {
    const auto bare_vm_result = run_cli(t81_bin, {"vm"});
    T81_TEST_CHECK(bare_vm_result.exit_code == 0);
    T81_TEST_CHECK(contains(bare_vm_result.stdout_text, "Usage: t81 vm <action> [args]"));

    const auto bare_trace_result = run_cli(t81_bin, {"trace"});
    T81_TEST_CHECK(bare_trace_result.exit_code == 0);
    T81_TEST_CHECK(contains(bare_trace_result.stdout_text, "Usage: t81 trace <subcommand> [args]"));

    const auto completion_result = run_cli(t81_bin, {"completion", "fish"});
    T81_TEST_CHECK(completion_result.exit_code == 0);
    T81_TEST_CHECK(contains(completion_result.stdout_text, "__fish_seen_subcommand_from c"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "__fish_seen_subcommand_from rust"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "__fish_seen_subcommand_from python"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "__fish_seen_subcommand_from llvm"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "__fish_seen_subcommand_from mlir"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "compile help"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "compile lower pipeline help"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "fsck"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "repair"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "__fish_seen_subcommand_from ai"));
    T81_TEST_CHECK(contains(completion_result.stdout_text, "backend model verify inference"));
    T81_TEST_CHECK(!contains(completion_result.stdout_text, " bench "));
  }

  {
    const fs::path hello_world = fs::absolute(t81_bin).parent_path().parent_path() / "examples" /
                                 "core-language" / "hello_world.t81";

    const auto llvm_result = run_cli(t81_bin, {"llvm", "compile", hello_world.string()});
    T81_TEST_CHECK(!contains(llvm_result.stderr_text, "Multiple input files not supported"));
    T81_TEST_CHECK(llvm_result.exit_code == 0 ||
                   contains(llvm_result.stderr_text, "built without the LLVM backend."));

    const auto mlir_result = run_cli(t81_bin, {"mlir", "compile", hello_world.string()});
    T81_TEST_CHECK(!contains(mlir_result.stderr_text, "Multiple input files not supported"));
    T81_TEST_CHECK(mlir_result.exit_code == 0 ||
                   contains(mlir_result.stderr_text, "built without the MLIR frontend."));

    const auto mlir_t81_dialect_result =
        run_cli(t81_bin, {"mlir", "compile", hello_world.string(), "--dialect=t81"});
    T81_TEST_CHECK(
        !contains(mlir_t81_dialect_result.stderr_text, "Multiple input files not supported"));
    T81_TEST_CHECK(
        mlir_t81_dialect_result.exit_code == 0 ||
        contains(mlir_t81_dialect_result.stderr_text, "built without the MLIR frontend."));

    const auto c_help_result = run_cli(t81_bin, {"help", "c"});
    T81_TEST_CHECK(c_help_result.exit_code == 0);
    T81_TEST_CHECK(contains(c_help_result.stdout_text, "Usage: t81 c compile"));

    const auto rust_help_result = run_cli(t81_bin, {"help", "rust"});
    T81_TEST_CHECK(rust_help_result.exit_code == 0);
    T81_TEST_CHECK(contains(rust_help_result.stdout_text, "Usage: t81 rust compile"));

    const auto python_help_result = run_cli(t81_bin, {"help", "python"});
    T81_TEST_CHECK(python_help_result.exit_code == 0);
    T81_TEST_CHECK(contains(python_help_result.stdout_text, "Usage: t81 python compile"));

    const fs::path c_input = make_temp_path("t81-cli-contract", ".c");
    const fs::path c_output = make_temp_path("t81-cli-contract", ".mlir");
    {
      std::ofstream out(c_input);
      out << "int main() {\n"
             "  int x = 2;\n"
             "  int y = 3;\n"
             "  return x + y;\n"
             "}\n";
    }
    const auto c_result = run_cli(
        t81_bin, {"c", "compile", c_input.string(), "-o", c_output.string(), "--dialect=t81"});
    T81_TEST_CHECK(!contains(c_result.stderr_text, "Multiple input files not supported"));
    T81_TEST_CHECK(c_result.exit_code == 0 ||
                   contains(c_result.stderr_text, "built without the experimental C frontend."));

    const fs::path rust_input = make_temp_path("t81-cli-contract", ".rs");
    const fs::path rust_output = make_temp_path("t81-cli-contract", ".mlir");
    {
      std::ofstream out(rust_input);
      out << "fn main() -> i32 {\n"
             "    let x: i32 = 2;\n"
             "    if x == 2 {\n"
             "        return x + 3;\n"
             "    }\n"
             "    return 0;\n"
             "}\n";
    }
    const auto rust_result =
        run_cli(t81_bin, {"rust", "compile", rust_input.string(), "-o", rust_output.string()});
    T81_TEST_CHECK(rust_result.exit_code != 0 ||
                   contains(rust_result.stdout_text, "MLIR written to:"));
    T81_TEST_CHECK(
        contains(rust_result.stderr_text, "built without the experimental Rust frontend.") ||
        rust_result.exit_code == 0);

    const fs::path python_input = make_temp_path("t81-cli-contract", ".py");
    const fs::path python_output = make_temp_path("t81-cli-contract", ".mlir");
    {
      std::ofstream out(python_input);
      out << "def main() -> int:\n"
             "    x: int = 2\n"
             "    if x == 2:\n"
             "        return x + 3\n"
             "    return 0\n";
    }
    const auto python_result = run_cli(
        t81_bin, {"python", "compile", python_input.string(), "-o", python_output.string()});
    T81_TEST_CHECK(python_result.exit_code != 0 ||
                   contains(python_result.stdout_text, "MLIR written to:"));
    T81_TEST_CHECK(
        contains(python_result.stderr_text, "built without the experimental Python frontend.") ||
        python_result.exit_code == 0);
    std::error_code ignore_ec;
    fs::remove(c_input, ignore_ec);
    fs::remove(c_output, ignore_ec);
    fs::remove(rust_input, ignore_ec);
    fs::remove(rust_output, ignore_ec);
    fs::remove(python_input, ignore_ec);
    fs::remove(python_output, ignore_ec);
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

  {
    const fs::path model_dir = make_temp_path("t81-cli-classify-fixed-model-dir", "");
    const fs::path canonfs_root = make_temp_path("t81-cli-classify-fixed-canonfs", "");
    const fs::path ruleset_dir = make_temp_path("t81-cli-classify-fixed-rulesets", "");
    fs::create_directories(canonfs_root);
    fs::create_directories(ruleset_dir);

    const fs::path builder = fs::absolute(t81_bin).parent_path() / "t81_make_classify_fixed_demo";
    T81_TEST_CHECK(fs::exists(builder));
    const auto build_result = run_cli(builder, {model_dir.string()});
    T81_TEST_CHECK(build_result.exit_code == 0);

    const fs::path model_path = model_dir / "classify-fixed-demo.t81w";
    const auto hash_result = run_cli(t81_bin, {"determinism", "hash", model_path.string()});
    T81_TEST_CHECK(hash_result.exit_code == 0);
    std::istringstream hash_stream(hash_result.stdout_text);
    std::string raw_model_hash;
    hash_stream >> raw_model_hash;
    T81_TEST_CHECK(!raw_model_hash.empty());

    const fs::path allow_policy = make_temp_path("t81-cli-classify-fixed-allow", ".apl");
    {
      std::ofstream out(allow_policy);
      out << "(policy\n"
             "  (tier 1)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:"
          << raw_model_hash
          << "\"])\n"
             "  (require-axion-event (reason \"task:classify_fixed.v1\")))\n";
    }

    const auto task_result =
        run_cli(t81_bin, {"ai", "task", "classify-fixed", "--model", "classify-fixed-demo",
                          "--model-file", model_path.string(), "--policy", allow_policy.string(),
                          "--canonfs-root", canonfs_root.string(), "--mode", "strict_deterministic",
                          "--input", "greet hello"});
    T81_TEST_CHECK(task_result.exit_code == 0);
    const auto result_ref = extract_json_string(task_result.stdout_text, "result_ref");
    const auto provenance_ref = extract_json_string(task_result.stdout_text, "provenance_ref");
    T81_TEST_CHECK(result_ref.has_value());
    T81_TEST_CHECK(provenance_ref.has_value());

    const fs::path ruleset_path = ruleset_dir / "positive.ruleset";
    {
      std::ofstream out(ruleset_path);
      out << "label=POSITIVE\n"
          << "selected_rule_set=positive-default\n"
          << "source_result_ref=" << *result_ref << "\n";
    }
    const auto action_ref_result = run_cli(t81_bin, {"canonfs", "put-file", ruleset_path.string(),
                                                     "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(action_ref_result.exit_code == 0);
    std::string action_ref = action_ref_result.stdout_text;
    while (!action_ref.empty() && (action_ref.back() == '\n' || action_ref.back() == '\r')) {
      action_ref.pop_back();
    }
    T81_TEST_CHECK(!action_ref.empty());

    const auto write_store_result =
        run_cli(t81_bin, {"artifact", "write-store-record", "--schema",
                          "t81.ai.task.classify-fixed.rule-selection-record.v1", "--field",
                          "source_result_ref=" + *result_ref, "--field",
                          "source_provenance_ref=" + *provenance_ref, "--field", "label=POSITIVE",
                          "--field", "termination_reason=single_step_max_score", "--field",
                          "selected_rule_set=positive-default", "--field",
                          "rule_set_ref=" + action_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(write_store_result.exit_code == 0);
    T81_TEST_CHECK(contains(write_store_result.stdout_text,
                            "\"schema\": \"t81.artifact.record-write-store.v1\""));
    T81_TEST_CHECK(appears_in_order(
        write_store_result.stdout_text,
        {"\"result_summary\": ", "\"schema_id\": ", "\"validation_result\": \"pass\"",
         "\"record_ref\": ", "\"next_step_hint\": ", "\"status\": \"pass\""}));
    const auto record_ref = extract_json_string(write_store_result.stdout_text, "record_ref");
    T81_TEST_CHECK(record_ref.has_value());

    const auto bundle_store_result = run_cli(
        t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.classify-fixed.bundle.v1",
                  "--field", "source_result_ref=" + *result_ref, "--field",
                  "source_provenance_ref=" + *provenance_ref, "--field", "action_ref=" + action_ref,
                  "--field", "record_ref=" + *record_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(bundle_store_result.exit_code == 0);
    T81_TEST_CHECK(
        contains(bundle_store_result.stdout_text, "\"schema\": \"t81.artifact.bundle-store.v1\""));
    T81_TEST_CHECK(appears_in_order(
        bundle_store_result.stdout_text,
        {"\"result_summary\": ", "\"schema_id\": ", "\"validation_result\": \"pass\"",
         "\"bundle_ref\": ", "\"next_step_hint\": ", "\"status\": \"pass\""}));
    const auto bundle_ref = extract_json_string(bundle_store_result.stdout_text, "bundle_ref");
    T81_TEST_CHECK(bundle_ref.has_value());

    const auto bundle_artifact =
        run_cli(t81_bin, {"canonfs", "get", *bundle_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(bundle_artifact.exit_code == 0);
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text,
                            "\"schema\": \"t81.ai.task.classify-fixed.bundle.v1\""));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"source_result_ref\": "));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"source_provenance_ref\": "));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"action_ref\": "));
    T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"record_ref\": "));

    std::error_code ignore_ec;
    fs::remove(allow_policy, ignore_ec);
    fs::remove_all(model_dir, ignore_ec);
    fs::remove_all(canonfs_root, ignore_ec);
    fs::remove_all(ruleset_dir, ignore_ec);
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
    const auto ok_result =
        run_cli(t81_bin, {"internal", "pkg", "check", package_file.string(), "--json"});
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
    const auto bad_result =
        run_cli(t81_bin, {"internal", "pkg", "check", bad_file.string(), "--json"});
    T81_TEST_CHECK(bad_result.exit_code == 2);
    T81_TEST_CHECK(contains(bad_result.stdout_text, "\"valid\": false"));

    // Clean up
    fs::remove(package_file, ignore_ec);
    fs::remove(bad_file, ignore_ec);
  }

  std::cout << "CLI contract tests PASSED!\n";
  return 0;
}
