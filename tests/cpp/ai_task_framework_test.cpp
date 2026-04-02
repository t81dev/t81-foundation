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

struct TaskCase {
  std::string cli_name;
  std::string model_name;
  std::string builder_name;
  std::string model_file_name;
  std::string policy_marker;
  std::string input_text;
  std::string run_schema;
  std::string result_schema;
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
  const fs::path out_path = make_temp_path("t81-ai-task-framework", ".out");
  const fs::path err_path = make_temp_path("t81-ai-task-framework", ".err");

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
  T81_TEST_CHECK(fs::exists(t81_bin));
  const fs::path build_dir = t81_bin.parent_path();

  const std::vector<TaskCase> tasks = {
      {"answer-fixed", "answer-fixed-demo", "t81_make_answer_fixed_demo",
       "answer-fixed-demo.t81w", "task:answer_fixed.v1", "greet hello",
       "t81.ai.task-run.answer-fixed.v1", "t81.ai.task.answer-fixed.v1"},
      {"classify-fixed", "classify-fixed-demo", "t81_make_classify_fixed_demo",
       "classify-fixed-demo.t81w", "task:classify_fixed.v1", "greet hello",
       "t81.ai.task-run.classify-fixed.v1", "t81.ai.task.classify-fixed.v1"},
      {"route-fixed", "route-fixed-demo", "t81_make_route_fixed_demo",
       "route-fixed-demo.t81w", "task:route_fixed.v1", "greet hello",
       "t81.ai.task-run.route-fixed.v1", "t81.ai.task.route-fixed.v1"},
      {"assess-fixed", "assess-fixed-demo", "t81_make_assess_fixed_demo",
       "assess-fixed-demo.t81w", "task:assess_fixed.v1", "greet hello",
       "t81.ai.task-run.assess-fixed.v1", "t81.ai.task.assess-fixed.v1"},
  };

  for (const auto& task : tasks) {
    const fs::path tmp_root = make_temp_path("t81-ai-task-fw", "");
    const fs::path model_dir = tmp_root / "model";
    const fs::path canonfs_root = tmp_root / ".t81_canonfs";
    fs::create_directories(canonfs_root);

    const fs::path builder = build_dir / task.builder_name;
    T81_TEST_CHECK(fs::exists(builder));
    const auto build_result = run_cli(builder, {model_dir.string()});
    T81_TEST_CHECK(build_result.exit_code == 0);

    const fs::path model_path = model_dir / task.model_file_name;
    const auto hash_result = run_cli(t81_bin, {"determinism", "hash", model_path.string()});
    T81_TEST_CHECK(hash_result.exit_code == 0);
    std::istringstream hash_stream(hash_result.stdout_text);
    std::string raw_model_hash;
    hash_stream >> raw_model_hash;
    T81_TEST_CHECK(!raw_model_hash.empty());

    const fs::path allow_policy = tmp_root / "allow.apl";
    {
      std::ofstream out(allow_policy);
      out << "(policy\n"
             "  (tier 1)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:" << raw_model_hash << "\"])\n"
             "  (require-axion-event (reason \"" << task.policy_marker << "\")))\n";
    }

    const auto first = run_cli(
        t81_bin, {"ai", "task", task.cli_name, "--model", task.model_name, "--model-file",
                  model_path.string(), "--policy", allow_policy.string(), "--canonfs-root",
                  canonfs_root.string(), "--mode", "strict_deterministic", "--input",
                  task.input_text});
    const auto second = run_cli(
        t81_bin, {"ai", "task", task.cli_name, "--model", task.model_name, "--model-file",
                  model_path.string(), "--policy", allow_policy.string(), "--canonfs-root",
                  canonfs_root.string(), "--mode", "strict_deterministic", "--input",
                  task.input_text});
    T81_TEST_CHECK(first.exit_code == 0);
    T81_TEST_CHECK(second.exit_code == 0);
    T81_TEST_CHECK(contains(first.stdout_text, "\"schema\": \"" + task.run_schema + "\""));
    T81_TEST_CHECK(contains(first.stdout_text, "\"result_summary\": "));
    T81_TEST_CHECK(contains(first.stdout_text, "\"result_class\": \"ai_task_result\""));
    T81_TEST_CHECK(contains(first.stdout_text, "\"result_ref\": "));
    T81_TEST_CHECK(contains(first.stdout_text, "\"provenance_ref\": "));
    T81_TEST_CHECK(contains(first.stdout_text, "\"policy_result\": \"allowed\""));
    T81_TEST_CHECK(contains(first.stdout_text, "\"termination_reason\": "));

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

    const auto result_artifact =
        run_cli(t81_bin, {"canonfs", "get", *first_result_ref, "--canonfs-root", canonfs_root.string()});
    const auto provenance_artifact =
        run_cli(t81_bin, {"canonfs", "get", *first_provenance_ref, "--canonfs-root", canonfs_root.string()});
    T81_TEST_CHECK(result_artifact.exit_code == 0);
    T81_TEST_CHECK(provenance_artifact.exit_code == 0);
    T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"schema\": \"" + task.result_schema + "\""));
    T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"task\": "));
    T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"termination_reason\": "));
    T81_TEST_CHECK(contains(provenance_artifact.stdout_text, "\"schema\": \"t81.ai.task.provenance.v1\""));
    T81_TEST_CHECK(contains(provenance_artifact.stdout_text, "\"policy_result\": \"allowed\""));

    const fs::path deny_policy = tmp_root / "deny.apl";
    {
      std::ofstream out(deny_policy);
      out << "(policy\n"
             "  (tier 1)\n"
             "  (allowed-ternary-model-hashes [\"sha3-512:deadbeef\"])\n"
             "  (require-axion-event (reason \"" << task.policy_marker << "\")))\n";
    }
    const auto denied = run_cli(
        t81_bin, {"ai", "task", task.cli_name, "--model", task.model_name, "--model-file",
                  model_path.string(), "--policy", deny_policy.string(), "--canonfs-root",
                  canonfs_root.string(), "--mode", "strict_deterministic", "--input",
                  task.input_text});
    T81_TEST_CHECK(denied.exit_code == 13);
    T81_TEST_CHECK(contains(denied.stderr_text, "SecurityFault"));
  }

  return 0;
}
