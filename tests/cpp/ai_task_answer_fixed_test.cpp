#include "t81/weights.hpp"
#include "test_runtime_check.hpp"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct CommandResult {
  std::string stdout_text;
  std::string stderr_text;
  int exit_code = 0;
};

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

CommandResult run_cli(const fs::path& bin_path, const std::vector<std::string>& args,
                      std::chrono::seconds timeout = std::chrono::seconds(20)) {
  const fs::path out_path = make_temp_path("t81-answer-fixed-test", ".out");
  const fs::path err_path = make_temp_path("t81-answer-fixed-test", ".err");

  std::vector<std::string> argv_storage;
  argv_storage.push_back(fs::absolute(bin_path).string());
  argv_storage.insert(argv_storage.end(), args.begin(), args.end());

  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(argv_storage.size() + 1);
  for (auto& item : argv_storage) {
    argv_ptrs.push_back(item.data());
  }
  argv_ptrs.push_back(nullptr);

  pid_t pid = fork();
  T81_TEST_CHECK(pid >= 0);
  if (pid == 0) {
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

}  // namespace

int main(int argc, char* argv[]) {
  T81_TEST_CHECK(argc >= 2);
  const fs::path t81_bin = fs::path(argv[1]);
  T81_TEST_CHECK(fs::exists(t81_bin));

  const fs::path model_dir = make_temp_path("t81-answer-fixed-model-dir", "");
  write_answer_fixed_demo_model(model_dir);
  const fs::path model_path = model_dir / "answer-fixed-demo.t81w";
  const fs::path canonfs_root = make_temp_path("t81-answer-fixed-canonfs", "");
  fs::create_directories(canonfs_root);

  const auto hash_result = run_cli(t81_bin, {"determinism", "hash", model_path.string()});
  T81_TEST_CHECK(hash_result.exit_code == 0);
  std::istringstream hash_stream(hash_result.stdout_text);
  std::string raw_model_hash;
  hash_stream >> raw_model_hash;
  T81_TEST_CHECK(!raw_model_hash.empty());

  const fs::path allow_policy = make_temp_path("t81-answer-fixed-allow", ".apl");
  {
    std::ofstream out(allow_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:" << raw_model_hash << "\"])\n"
           "  (require-axion-event (reason \"task:answer_fixed.v1\")))\n";
  }

  const fs::path deny_policy = make_temp_path("t81-answer-fixed-deny", ".apl");
  {
    std::ofstream out(deny_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:deadbeef\"])\n"
           "  (require-axion-event (reason \"task:answer_fixed.v1\")))\n";
  }

  const fs::path missing_marker_policy = make_temp_path("t81-answer-fixed-missing-marker", ".apl");
  {
    std::ofstream out(missing_marker_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:" << raw_model_hash << "\"]))\n";
  }

  const fs::path unsupported_model_path = make_temp_path("t81-answer-fixed-unsupported", ".t81w");
  {
    t81::weights::NativeModel unsupported_model;
    t81::weights::NativeTensor mat_a;
    mat_a.shape = {2, 2};
    mat_a.trits = 4;
    mat_a.data = {1};
    unsupported_model["mat_a"] = mat_a;

    t81::weights::NativeTensor mat_b;
    mat_b.shape = {2, 2};
    mat_b.trits = 4;
    mat_b.data = {2};
    unsupported_model["mat_b"] = mat_b;
    t81::weights::save_t81w(unsupported_model, unsupported_model_path);
  }
  const auto unsupported_hash_result =
      run_cli(t81_bin, {"determinism", "hash", unsupported_model_path.string()});
  T81_TEST_CHECK(unsupported_hash_result.exit_code == 0);
  std::istringstream unsupported_hash_stream(unsupported_hash_result.stdout_text);
  std::string unsupported_model_hash;
  unsupported_hash_stream >> unsupported_model_hash;
  T81_TEST_CHECK(!unsupported_model_hash.empty());
  const fs::path unsupported_policy = make_temp_path("t81-answer-fixed-unsupported", ".apl");
  {
    std::ofstream out(unsupported_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:" << unsupported_model_hash << "\"])\n"
           "  (require-axion-event (reason \"task:answer_fixed.v1\")))\n";
  }

  const std::vector<std::string> base_args = {
      "ai",          "task",        "answer-fixed", "--model",      "answer-fixed-demo",
      "--model-file", model_path.string(), "--policy", allow_policy.string(), "--canonfs-root",
      canonfs_root.string(), "--mode", "strict_deterministic"};

  auto yes_args = base_args;
  yes_args.push_back("--input");
  yes_args.push_back("greet hello");

  const auto missing_policy_result =
      run_cli(t81_bin, {"ai", "task", "answer-fixed", "--model", "answer-fixed-demo",
                        "--model-file", model_path.string(), "--mode",
                        "strict_deterministic", "--input", "greet hello"});
  T81_TEST_CHECK(missing_policy_result.exit_code != 0);
  T81_TEST_CHECK(contains(missing_policy_result.stderr_text,
                          "error: answer-fixed requires --policy <file>"));

  const auto non_strict_result =
      run_cli(t81_bin, {"ai", "task", "answer-fixed", "--model", "answer-fixed-demo",
                        "--model-file", model_path.string(), "--policy", allow_policy.string(),
                        "--canonfs-root", canonfs_root.string(), "--mode",
                        "reproducible_nondeterministic", "--input", "greet hello"});
  T81_TEST_CHECK(non_strict_result.exit_code != 0);
  T81_TEST_CHECK(contains(non_strict_result.stderr_text,
                          "error: answer-fixed requires --mode strict_deterministic"));

  const auto unsupported_result =
      run_cli(t81_bin, {"ai", "task", "answer-fixed", "--model", "unsupported-demo",
                        "--model-file", unsupported_model_path.string(), "--policy",
                        unsupported_policy.string(), "--canonfs-root", canonfs_root.string(), "--mode",
                        "strict_deterministic", "--input", "greet hello"});
  T81_TEST_CHECK(unsupported_result.exit_code != 0);
  T81_TEST_CHECK(contains(
      unsupported_result.stderr_text,
      "error: answer-fixed supports only llama-dense-v1 narrow greedy decode"));

  const auto first = run_cli(t81_bin, yes_args);
  const auto second = run_cli(t81_bin, yes_args);
  T81_TEST_CHECK(first.exit_code == 0);
  T81_TEST_CHECK(second.exit_code == 0);
  T81_TEST_CHECK(contains(first.stdout_text,
                          "\"next_step_hint\": \"next (progress): t81 canonfs get "));

  const auto first_result_ref = extract_json_string(first.stdout_text, "result_ref");
  const auto second_result_ref = extract_json_string(second.stdout_text, "result_ref");
  const auto first_provenance_ref = extract_json_string(first.stdout_text, "provenance_ref");
  const auto second_provenance_ref = extract_json_string(second.stdout_text, "provenance_ref");
  T81_TEST_CHECK(first_result_ref.has_value());
  T81_TEST_CHECK(second_result_ref.has_value());
  T81_TEST_CHECK(first_provenance_ref.has_value());
  T81_TEST_CHECK(second_provenance_ref.has_value());
  T81_TEST_CHECK(*first_result_ref == *second_result_ref);
  T81_TEST_CHECK(*first_provenance_ref == *second_provenance_ref);

  const auto first_result_artifact =
      run_cli(t81_bin, {"canonfs", "get", *first_result_ref, "--canonfs-root", canonfs_root.string()});
  const auto second_result_artifact =
      run_cli(t81_bin, {"canonfs", "get", *second_result_ref, "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(first_result_artifact.exit_code == 0);
  T81_TEST_CHECK(second_result_artifact.exit_code == 0);
  T81_TEST_CHECK(first_result_artifact.stdout_text == second_result_artifact.stdout_text);
  T81_TEST_CHECK(contains(first_result_artifact.stdout_text, "\"answer\": \"YES\""));
  T81_TEST_CHECK(contains(first_result_artifact.stdout_text, "\"answer_token_ids\": [42]"));

  auto no_args = base_args;
  no_args.push_back("--input");
  no_args.push_back("hello world");
  const auto no_result = run_cli(t81_bin, no_args);
  T81_TEST_CHECK(no_result.exit_code == 0);
  T81_TEST_CHECK(contains(no_result.stdout_text, "\"answer\": \"NO\""));
  T81_TEST_CHECK(contains(no_result.stdout_text, "\"answer_token_ids\": [9]"));

  const auto deny_result =
      run_cli(t81_bin, {"ai", "task", "answer-fixed", "--model", "answer-fixed-demo",
                        "--model-file", model_path.string(), "--policy", deny_policy.string(),
                        "--canonfs-root", canonfs_root.string(), "--mode",
                        "strict_deterministic", "--input", "greet hello"});
  T81_TEST_CHECK(deny_result.exit_code == 13);
  T81_TEST_CHECK(contains(deny_result.stderr_text, "SecurityFault"));
  T81_TEST_CHECK(contains(deny_result.stderr_text,
                          "allowed-ternary-model-hashes entry matching model checksum"));

  const auto missing_marker_result =
      run_cli(t81_bin, {"ai", "task", "answer-fixed", "--model", "answer-fixed-demo",
                        "--model-file", model_path.string(), "--policy",
                        missing_marker_policy.string(), "--canonfs-root", canonfs_root.string(),
                        "--mode", "strict_deterministic", "--input", "greet hello"});
  T81_TEST_CHECK(missing_marker_result.exit_code == 13);
  T81_TEST_CHECK(contains(missing_marker_result.stderr_text, "SecurityFault"));
  T81_TEST_CHECK(contains(missing_marker_result.stderr_text,
                          "policy is missing require-axion-event reason"));

  std::error_code ignore_ec;
  fs::remove(allow_policy, ignore_ec);
  fs::remove(deny_policy, ignore_ec);
  fs::remove(missing_marker_policy, ignore_ec);
  fs::remove(unsupported_policy, ignore_ec);
  fs::remove(unsupported_model_path, ignore_ec);
  fs::remove_all(model_dir, ignore_ec);
  fs::remove_all(canonfs_root, ignore_ec);
  return 0;
}
