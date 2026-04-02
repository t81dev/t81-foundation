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
  const fs::path out_path = make_temp_path("t81-ai-read-field", ".out");
  const fs::path err_path = make_temp_path("t81-ai-read-field", ".err");

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

}  // namespace

int main(int argc, char* argv[]) {
  T81_TEST_CHECK(argc >= 2);
  const fs::path t81_bin = fs::path(argv[1]);
  const fs::path build_dir = t81_bin.parent_path();

  const fs::path tmp_root = make_temp_path("t81-ai-read-field", "");
  const fs::path model_dir = tmp_root / "model";
  const fs::path canonfs_root = tmp_root / ".t81_canonfs";
  const fs::path local_result_path = tmp_root / "result.json";
  fs::create_directories(model_dir);
  fs::create_directories(canonfs_root);

  const fs::path builder = build_dir / "t81_make_assess_fixed_demo";
  T81_TEST_CHECK(fs::exists(builder));
  T81_TEST_CHECK(run_cli(builder, {model_dir.string()}).exit_code == 0);
  const fs::path answer_builder = build_dir / "t81_make_answer_fixed_demo";
  const fs::path classify_builder = build_dir / "t81_make_classify_fixed_demo";
  T81_TEST_CHECK(fs::exists(answer_builder));
  T81_TEST_CHECK(fs::exists(classify_builder));
  T81_TEST_CHECK(run_cli(answer_builder, {model_dir.string()}).exit_code == 0);
  T81_TEST_CHECK(run_cli(classify_builder, {model_dir.string()}).exit_code == 0);

  const fs::path model_path = model_dir / "assess-fixed-demo.t81w";
  const fs::path answer_model_path = model_dir / "answer-fixed-demo.t81w";
  const fs::path classify_model_path = model_dir / "classify-fixed-demo.t81w";
  const auto hash_result = run_cli(t81_bin, {"determinism", "hash", model_path.string()});
  T81_TEST_CHECK(hash_result.exit_code == 0);
  std::istringstream hash_stream(hash_result.stdout_text);
  std::string raw_model_hash;
  hash_stream >> raw_model_hash;
  T81_TEST_CHECK(!raw_model_hash.empty());
  const auto answer_hash_result = run_cli(t81_bin, {"determinism", "hash", answer_model_path.string()});
  const auto classify_hash_result =
      run_cli(t81_bin, {"determinism", "hash", classify_model_path.string()});
  T81_TEST_CHECK(answer_hash_result.exit_code == 0);
  T81_TEST_CHECK(classify_hash_result.exit_code == 0);
  std::istringstream answer_hash_stream(answer_hash_result.stdout_text);
  std::istringstream classify_hash_stream(classify_hash_result.stdout_text);
  std::string answer_model_hash;
  std::string classify_model_hash;
  answer_hash_stream >> answer_model_hash;
  classify_hash_stream >> classify_model_hash;
  T81_TEST_CHECK(!answer_model_hash.empty());
  T81_TEST_CHECK(!classify_model_hash.empty());

  const fs::path allow_policy = tmp_root / "allow.apl";
  {
    std::ofstream out(allow_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:" << raw_model_hash << "\"])\n"
           "  (require-axion-event (reason \"task:assess_fixed.v1\")))\n";
  }
  const fs::path answer_policy = tmp_root / "answer-allow.apl";
  {
    std::ofstream out(answer_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:" << answer_model_hash << "\"])\n"
           "  (require-axion-event (reason \"task:answer_fixed.v1\")))\n";
  }
  const fs::path classify_policy = tmp_root / "classify-allow.apl";
  {
    std::ofstream out(classify_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:" << classify_model_hash << "\"])\n"
           "  (require-axion-event (reason \"task:classify_fixed.v1\")))\n";
  }

  const auto task_result = run_cli(
      t81_bin, {"ai", "task", "assess-fixed", "--model", "assess-fixed-demo", "--model-file",
                model_path.string(), "--policy", allow_policy.string(), "--canonfs-root",
                canonfs_root.string(), "--mode", "strict_deterministic", "--input",
                "greet hello"});
  T81_TEST_CHECK(task_result.exit_code == 0);
  const auto result_ref = extract_json_string(task_result.stdout_text, "result_ref");
  const auto provenance_ref = extract_json_string(task_result.stdout_text, "provenance_ref");
  T81_TEST_CHECK(result_ref.has_value());
  T81_TEST_CHECK(provenance_ref.has_value());

  const auto decision_from_ref = run_cli(
      t81_bin, {"ai", "task", "read-field", *result_ref, "--field", "decision", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(decision_from_ref.exit_code == 0);
  T81_TEST_CHECK(decision_from_ref.stdout_text == "ALLOW\n");

  const auto export_result = run_cli(
      t81_bin, {"canonfs", "get", *result_ref, "--canonfs-root", canonfs_root.string(), "--out",
                local_result_path.string(), "--json"});
  T81_TEST_CHECK(export_result.exit_code == 0);

  const auto reason_from_file =
      run_cli(t81_bin, {"ai", "task", "read-field", local_result_path.string(), "--field", "reason_code"});
  T81_TEST_CHECK(reason_from_file.exit_code == 0);
  T81_TEST_CHECK(reason_from_file.stdout_text == "GREETING_PAIR\n");

  const auto first_repeat = run_cli(
      t81_bin, {"ai", "task", "read-field", *result_ref, "--field", "termination_reason",
                "--canonfs-root", canonfs_root.string()});
  const auto second_repeat = run_cli(
      t81_bin, {"ai", "task", "read-field", *result_ref, "--field", "termination_reason",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(first_repeat.exit_code == 0);
  T81_TEST_CHECK(second_repeat.exit_code == 0);
  T81_TEST_CHECK(first_repeat.stdout_text == second_repeat.stdout_text);
  T81_TEST_CHECK(first_repeat.stdout_text == "single_step_max_score\n");

  const auto missing_field = run_cli(
      t81_bin, {"ai", "task", "read-field", *result_ref, "--field", "label", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(missing_field.exit_code != 0);
  T81_TEST_CHECK(contains(missing_field.stderr_text, "field \"label\" not present"));

  const auto wrong_schema = run_cli(
      t81_bin, {"ai", "task", "read-field", *provenance_ref, "--field", "task", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(wrong_schema.exit_code != 0);
  T81_TEST_CHECK(
      contains(wrong_schema.stderr_text, "supports only canonical AI task result artifacts"));

  const auto answer_task = run_cli(
      t81_bin, {"ai", "task", "answer-fixed", "--model", "answer-fixed-demo", "--model-file",
                answer_model_path.string(), "--policy", answer_policy.string(), "--canonfs-root",
                canonfs_root.string(), "--mode", "strict_deterministic", "--input",
                "greet hello"});
  T81_TEST_CHECK(answer_task.exit_code == 0);
  const auto answer_ref = extract_json_string(answer_task.stdout_text, "result_ref");
  T81_TEST_CHECK(answer_ref.has_value());
  const auto answer_value = run_cli(
      t81_bin, {"ai", "task", "read-field", *answer_ref, "--field", "answer", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(answer_value.exit_code == 0);
  T81_TEST_CHECK(answer_value.stdout_text == "YES\n");
  const auto answer_schema = run_cli(
      t81_bin, {"ai", "task", "read-field", *answer_ref, "--field", "schema", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(answer_schema.exit_code == 0);
  T81_TEST_CHECK(answer_schema.stdout_text == "t81.ai.task.answer-fixed.v1\n");

  const auto classify_task = run_cli(
      t81_bin, {"ai", "task", "classify-fixed", "--model", "classify-fixed-demo", "--model-file",
                classify_model_path.string(), "--policy", classify_policy.string(), "--canonfs-root",
                canonfs_root.string(), "--mode", "strict_deterministic", "--input",
                "greet hello"});
  T81_TEST_CHECK(classify_task.exit_code == 0);
  const auto classify_ref = extract_json_string(classify_task.stdout_text, "result_ref");
  T81_TEST_CHECK(classify_ref.has_value());
  const auto classify_label = run_cli(
      t81_bin, {"ai", "task", "read-field", *classify_ref, "--field", "label", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(classify_label.exit_code == 0);
  T81_TEST_CHECK(classify_label.stdout_text == "POSITIVE\n");
  const auto classify_task_name = run_cli(
      t81_bin, {"ai", "task", "read-field", *classify_ref, "--field", "task", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(classify_task_name.exit_code == 0);
  T81_TEST_CHECK(classify_task_name.stdout_text == "classify_fixed\n");

  return 0;
}
