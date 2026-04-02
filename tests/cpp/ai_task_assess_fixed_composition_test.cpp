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
  const fs::path out_path = make_temp_path("t81-assess-compose", ".out");
  const fs::path err_path = make_temp_path("t81-assess-compose", ".err");

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
  T81_TEST_CHECK(fs::exists(t81_bin));

  const fs::path tmp_root = make_temp_path("t81-assess-compose", "");
  const fs::path model_dir = tmp_root / "model";
  const fs::path canonfs_root = tmp_root / ".t81_canonfs";
  const fs::path canonfs_root_alt = tmp_root / ".t81_canonfs_alt";
  const fs::path action_dir = tmp_root / "actions";
  fs::create_directories(canonfs_root);
  fs::create_directories(canonfs_root_alt);
  fs::create_directories(action_dir);

  const fs::path builder = build_dir / "t81_make_assess_fixed_demo";
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

  const fs::path allow_policy = tmp_root / "allow.apl";
  {
    std::ofstream out(allow_policy);
    out << "(policy\n"
           "  (tier 1)\n"
           "  (allowed-ternary-model-hashes [\"sha3-512:"
        << raw_model_hash
        << "\"])\n"
           "  (require-axion-event (reason \"task:assess_fixed.v1\")))\n";
  }

  const auto task_result = run_cli(
      t81_bin, {"ai", "task", "assess-fixed", "--model", "assess-fixed-demo", "--model-file",
                model_path.string(), "--policy", allow_policy.string(), "--canonfs-root",
                canonfs_root.string(), "--mode", "strict_deterministic", "--input", "greet hello"});
  const auto repeat_task_result = run_cli(
      t81_bin, {"ai", "task", "assess-fixed", "--model", "assess-fixed-demo", "--model-file",
                model_path.string(), "--policy", allow_policy.string(), "--canonfs-root",
                canonfs_root.string(), "--mode", "strict_deterministic", "--input", "greet hello"});
  T81_TEST_CHECK(task_result.exit_code == 0);
  T81_TEST_CHECK(repeat_task_result.exit_code == 0);
  const auto result_ref = extract_json_string(task_result.stdout_text, "result_ref");
  const auto provenance_ref = extract_json_string(task_result.stdout_text, "provenance_ref");
  const auto repeat_result_ref = extract_json_string(repeat_task_result.stdout_text, "result_ref");
  const auto repeat_provenance_ref =
      extract_json_string(repeat_task_result.stdout_text, "provenance_ref");
  T81_TEST_CHECK(result_ref.has_value());
  T81_TEST_CHECK(provenance_ref.has_value());
  T81_TEST_CHECK(repeat_result_ref.has_value());
  T81_TEST_CHECK(repeat_provenance_ref.has_value());
  T81_TEST_CHECK(*result_ref == *repeat_result_ref);
  T81_TEST_CHECK(*provenance_ref == *repeat_provenance_ref);

  const auto cross_root_task_result =
      run_cli(t81_bin, {"ai", "task", "assess-fixed", "--model", "assess-fixed-demo",
                        "--model-file", model_path.string(), "--policy", allow_policy.string(),
                        "--canonfs-root", canonfs_root_alt.string(), "--mode",
                        "strict_deterministic", "--input", "greet hello"});
  T81_TEST_CHECK(cross_root_task_result.exit_code == 0);
  const auto cross_root_result_ref =
      extract_json_string(cross_root_task_result.stdout_text, "result_ref");
  const auto cross_root_provenance_ref =
      extract_json_string(cross_root_task_result.stdout_text, "provenance_ref");
  T81_TEST_CHECK(cross_root_result_ref.has_value());
  T81_TEST_CHECK(cross_root_provenance_ref.has_value());
  T81_TEST_CHECK(*result_ref == *cross_root_result_ref);
  T81_TEST_CHECK(*provenance_ref == *cross_root_provenance_ref);

  const auto result_artifact =
      run_cli(t81_bin, {"canonfs", "get", *result_ref, "--canonfs-root", canonfs_root.string()});
  const auto repeat_result_artifact = run_cli(
      t81_bin, {"canonfs", "get", *repeat_result_ref, "--canonfs-root", canonfs_root.string()});
  const auto cross_root_result_artifact = run_cli(
      t81_bin,
      {"canonfs", "get", *cross_root_result_ref, "--canonfs-root", canonfs_root_alt.string()});
  T81_TEST_CHECK(result_artifact.exit_code == 0);
  T81_TEST_CHECK(repeat_result_artifact.exit_code == 0);
  T81_TEST_CHECK(cross_root_result_artifact.exit_code == 0);
  T81_TEST_CHECK(result_artifact.stdout_text == repeat_result_artifact.stdout_text);
  T81_TEST_CHECK(result_artifact.stdout_text == cross_root_result_artifact.stdout_text);
  T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"decision\": \"ALLOW\""));
  T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"reason_code\": \"GREETING_PAIR\""));
  T81_TEST_CHECK(contains(result_artifact.stdout_text, "\"termination_reason\": "));

  const fs::path action_path = action_dir / "allow.marker";
  {
    std::ofstream out(action_path);
    out << "decision=ALLOW\n"
        << "reason_code=GREETING_PAIR\n"
        << "source_result_ref=" << *result_ref << "\n";
  }

  const auto action_ref = run_cli(t81_bin, {"canonfs", "put-file", action_path.string(),
                                            "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(action_ref.exit_code == 0);
  std::istringstream action_ref_stream(action_ref.stdout_text);
  std::string action_hash;
  action_ref_stream >> action_hash;
  T81_TEST_CHECK(!action_hash.empty());

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
                        "--field",        "action_ref=" + action_hash,
                        "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(write_store_result.exit_code == 0);
  T81_TEST_CHECK(contains(write_store_result.stdout_text,
                          "\"schema\": \"t81.artifact.record-write-store.v1\""));
  T81_TEST_CHECK(contains(write_store_result.stdout_text, "\"validation_result\": \"pass\""));
  const auto stored_record_ref = extract_json_string(write_store_result.stdout_text, "record_ref");
  T81_TEST_CHECK(stored_record_ref.has_value());
  std::istringstream record_ref_stream(*stored_record_ref);
  std::string record_hash;
  record_ref_stream >> record_hash;
  T81_TEST_CHECK(!record_hash.empty());

  const auto repeat_write_store_result =
      run_cli(t81_bin, {"artifact",       "write-store-record",
                        "--schema",       "t81.ai.task.assess-fixed.host-action-record.v1",
                        "--field",        "source_result_ref=" + *result_ref,
                        "--field",        "source_provenance_ref=" + *provenance_ref,
                        "--field",        "decision=ALLOW",
                        "--field",        "reason_code=GREETING_PAIR",
                        "--field",        "termination_reason=single_step_max_score",
                        "--field",        "selected_action=write_allow_marker",
                        "--field",        "selected_path=actions/allow.marker",
                        "--field",        "action_ref=" + action_hash,
                        "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(repeat_write_store_result.exit_code == 0);
  const auto repeat_record_ref =
      extract_json_string(repeat_write_store_result.stdout_text, "record_ref");
  T81_TEST_CHECK(repeat_record_ref.has_value());
  T81_TEST_CHECK(*stored_record_ref == *repeat_record_ref);

  const auto typed_record_field =
      run_cli(t81_bin, {"artifact", "read-field", record_hash, "--schema",
                        "t81.ai.task.assess-fixed.host-action-record.v1", "--field",
                        "selected_action", "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(typed_record_field.exit_code == 0);
  T81_TEST_CHECK(typed_record_field.stdout_text == "write_allow_marker\n");

  const fs::path action_path_alt = canonfs_root_alt / ".." / "actions-alt" / "allow.marker";
  fs::create_directories(action_path_alt.parent_path());
  {
    std::ofstream out(action_path_alt);
    out << "decision=ALLOW\n"
        << "reason_code=GREETING_PAIR\n"
        << "source_result_ref=" << *cross_root_result_ref << "\n";
  }

  const auto cross_root_action_ref =
      run_cli(t81_bin, {"canonfs", "put-file", action_path_alt.string(), "--canonfs-root",
                        canonfs_root_alt.string()});
  T81_TEST_CHECK(cross_root_action_ref.exit_code == 0);
  std::istringstream cross_root_action_ref_stream(cross_root_action_ref.stdout_text);
  std::string cross_root_action_hash;
  cross_root_action_ref_stream >> cross_root_action_hash;
  T81_TEST_CHECK(!cross_root_action_hash.empty());
  T81_TEST_CHECK(action_hash == cross_root_action_hash);

  const auto cross_root_write_store_result =
      run_cli(t81_bin, {"artifact",       "write-store-record",
                        "--schema",       "t81.ai.task.assess-fixed.host-action-record.v1",
                        "--field",        "source_result_ref=" + *cross_root_result_ref,
                        "--field",        "source_provenance_ref=" + *cross_root_provenance_ref,
                        "--field",        "decision=ALLOW",
                        "--field",        "reason_code=GREETING_PAIR",
                        "--field",        "termination_reason=single_step_max_score",
                        "--field",        "selected_action=write_allow_marker",
                        "--field",        "selected_path=actions/allow.marker",
                        "--field",        "action_ref=" + cross_root_action_hash,
                        "--canonfs-root", canonfs_root_alt.string()});
  T81_TEST_CHECK(cross_root_write_store_result.exit_code == 0);
  const auto cross_root_record_ref =
      extract_json_string(cross_root_write_store_result.stdout_text, "record_ref");
  T81_TEST_CHECK(cross_root_record_ref.has_value());
  T81_TEST_CHECK(*stored_record_ref == *cross_root_record_ref);

  const auto record_artifact =
      run_cli(t81_bin, {"canonfs", "get", record_hash, "--canonfs-root", canonfs_root.string()});
  const auto repeat_record_artifact = run_cli(
      t81_bin, {"canonfs", "get", *repeat_record_ref, "--canonfs-root", canonfs_root.string()});
  const auto cross_root_record_artifact = run_cli(
      t81_bin,
      {"canonfs", "get", *cross_root_record_ref, "--canonfs-root", canonfs_root_alt.string()});
  T81_TEST_CHECK(record_artifact.exit_code == 0);
  T81_TEST_CHECK(repeat_record_artifact.exit_code == 0);
  T81_TEST_CHECK(cross_root_record_artifact.exit_code == 0);
  T81_TEST_CHECK(record_artifact.stdout_text == repeat_record_artifact.stdout_text);
  T81_TEST_CHECK(record_artifact.stdout_text == cross_root_record_artifact.stdout_text);
  T81_TEST_CHECK(contains(record_artifact.stdout_text,
                          "\"schema\": \"t81.ai.task.assess-fixed.host-action-record.v1\""));
  T81_TEST_CHECK(contains(record_artifact.stdout_text, "\"decision\": \"ALLOW\""));
  T81_TEST_CHECK(
      contains(record_artifact.stdout_text, "\"selected_action\": \"write_allow_marker\""));
  T81_TEST_CHECK(contains(record_artifact.stdout_text, "\"action_ref\": "));
  T81_TEST_CHECK(contains(read_file(action_path), "decision=ALLOW"));

  const auto bundle_result = run_cli(
      t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.assess-fixed.bundle.v1",
                "--field", "source_result_ref=" + *result_ref, "--field",
                "source_provenance_ref=" + *provenance_ref, "--field", "action_ref=" + action_hash,
                "--field", "record_ref=" + record_hash, "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(bundle_result.exit_code == 0);
  T81_TEST_CHECK(
      contains(bundle_result.stdout_text, "\"schema\": \"t81.artifact.bundle-store.v1\""));
  T81_TEST_CHECK(contains(bundle_result.stdout_text, "\"validation_result\": \"pass\""));
  const auto bundle_ref = extract_json_string(bundle_result.stdout_text, "bundle_ref");
  T81_TEST_CHECK(bundle_ref.has_value());

  const auto repeat_bundle_result = run_cli(
      t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.assess-fixed.bundle.v1",
                "--field", "source_result_ref=" + *result_ref, "--field",
                "source_provenance_ref=" + *provenance_ref, "--field", "action_ref=" + action_hash,
                "--field", "record_ref=" + record_hash, "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(repeat_bundle_result.exit_code == 0);
  const auto repeat_bundle_ref =
      extract_json_string(repeat_bundle_result.stdout_text, "bundle_ref");
  T81_TEST_CHECK(repeat_bundle_ref.has_value());
  T81_TEST_CHECK(*bundle_ref == *repeat_bundle_ref);

  const auto cross_root_bundle_result = run_cli(
      t81_bin,
      {"artifact", "store-bundle", "--schema", "t81.ai.task.assess-fixed.bundle.v1", "--field",
       "source_result_ref=" + *cross_root_result_ref, "--field",
       "source_provenance_ref=" + *cross_root_provenance_ref, "--field",
       "action_ref=" + cross_root_action_hash, "--field", "record_ref=" + *cross_root_record_ref,
       "--canonfs-root", canonfs_root_alt.string()});
  T81_TEST_CHECK(cross_root_bundle_result.exit_code == 0);
  const auto cross_root_bundle_ref =
      extract_json_string(cross_root_bundle_result.stdout_text, "bundle_ref");
  T81_TEST_CHECK(cross_root_bundle_ref.has_value());
  T81_TEST_CHECK(*bundle_ref == *cross_root_bundle_ref);

  const auto bundle_record_ref =
      run_cli(t81_bin, {"artifact", "read-field", *bundle_ref, "--schema",
                        "t81.ai.task.assess-fixed.bundle.v1", "--field", "record_ref",
                        "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(bundle_record_ref.exit_code == 0);
  T81_TEST_CHECK(bundle_record_ref.stdout_text == record_hash + "\n");

  const auto bundle_artifact =
      run_cli(t81_bin, {"canonfs", "get", *bundle_ref, "--canonfs-root", canonfs_root.string()});
  const auto repeat_bundle_artifact = run_cli(
      t81_bin, {"canonfs", "get", *repeat_bundle_ref, "--canonfs-root", canonfs_root.string()});
  const auto cross_root_bundle_artifact = run_cli(
      t81_bin,
      {"canonfs", "get", *cross_root_bundle_ref, "--canonfs-root", canonfs_root_alt.string()});
  T81_TEST_CHECK(bundle_artifact.exit_code == 0);
  T81_TEST_CHECK(repeat_bundle_artifact.exit_code == 0);
  T81_TEST_CHECK(cross_root_bundle_artifact.exit_code == 0);
  T81_TEST_CHECK(bundle_artifact.stdout_text == repeat_bundle_artifact.stdout_text);
  T81_TEST_CHECK(bundle_artifact.stdout_text == cross_root_bundle_artifact.stdout_text);
  T81_TEST_CHECK(
      contains(bundle_artifact.stdout_text, "\"schema\": \"t81.ai.task.assess-fixed.bundle.v1\""));
  T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"source_result_ref\": "));
  T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"source_provenance_ref\": "));
  T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"action_ref\": "));
  T81_TEST_CHECK(contains(bundle_artifact.stdout_text, "\"record_ref\": "));

  return 0;
}
