#include "test_runtime_check.hpp"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
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
  const fs::path out_path = make_temp_path("t81-artifact-validate", ".out");
  const fs::path err_path = make_temp_path("t81-artifact-validate", ".err");

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

}  // namespace

int main(int argc, char* argv[]) {
  T81_TEST_CHECK(argc >= 2);
  const fs::path t81_bin = fs::path(argv[1]);

  const fs::path tmp_root = make_temp_path("t81-artifact-validate", "");
  const fs::path canonfs_root = tmp_root / ".t81_canonfs";
  const fs::path valid_record = tmp_root / "valid-record.json";
  const fs::path repeat_record = tmp_root / "repeat-record.json";
  const fs::path missing_field_record = tmp_root / "missing-field.json";
  const fs::path wrong_schema_record = tmp_root / "wrong-schema.json";
  fs::create_directories(canonfs_root);

  const std::vector<std::string> writer_args{
      "artifact",
      "write-record",
      "--schema",
      "t81.ai.task.assess-fixed.host-action-record.v1",
      "--out",
      valid_record.string(),
      "--field",
      "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
      "--field",
      "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
      "--field",
      "decision=ALLOW",
      "--field",
      "reason_code=GREETING_PAIR",
      "--field",
      "termination_reason=single_step_max_score",
      "--field",
      "selected_action=write_allow_marker",
      "--field",
      "selected_path=actions/allow.marker",
      "--field",
      "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
  };
  const auto write_valid = run_cli(t81_bin, writer_args);
  T81_TEST_CHECK(write_valid.exit_code == 0);
  T81_TEST_CHECK(contains(write_valid.stdout_text, "\"schema\": \"t81.artifact.record-write.v1\""));

  auto repeat_writer_args = writer_args;
  repeat_writer_args[5] = repeat_record.string();
  const auto write_repeat = run_cli(t81_bin, repeat_writer_args);
  T81_TEST_CHECK(write_repeat.exit_code == 0);
  T81_TEST_CHECK(read_file(valid_record) == read_file(repeat_record));

  {
    std::ofstream out(missing_field_record);
    out << "{\n"
           "  \"schema\": \"t81.ai.task.assess-fixed.host-action-record.v1\",\n"
           "  \"source_result_ref\": \"sha3-256:111111111111111111111111111111111111111111111111111111111\",\n"
           "  \"source_provenance_ref\": \"sha3-256:222222222222222222222222222222222222222222222222222222222\",\n"
           "  \"decision\": \"ALLOW\",\n"
           "  \"termination_reason\": \"single_step_max_score\",\n"
           "  \"selected_action\": \"write_allow_marker\",\n"
           "  \"selected_path\": \"actions/allow.marker\",\n"
           "  \"action_ref\": \"sha3-256:333333333333333333333333333333333333333333333333333333333\"\n"
           "}\n";
  }
  {
    std::ofstream out(wrong_schema_record);
    out << "{\n"
           "  \"schema\": \"t81.ai.task.answer-fixed.v1\",\n"
           "  \"task\": \"answer_fixed\",\n"
           "  \"termination_reason\": \"single_step_max_score\"\n"
           "}\n";
  }

  const auto valid = run_cli(
      t81_bin, {"artifact", "validate-record", valid_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1"});
  T81_TEST_CHECK(valid.exit_code == 0);
  T81_TEST_CHECK(contains(valid.stdout_text, "\"ok\": true"));
  T81_TEST_CHECK(
      contains(valid.stdout_text, "\"record_schema\": \"t81.ai.task.assess-fixed.host-action-record.v1\""));
  T81_TEST_CHECK(contains(valid.stdout_text, "\"selector_kind\": \"file\""));

  const auto repeat = run_cli(
      t81_bin, {"artifact", "validate-record", valid_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1"});
  T81_TEST_CHECK(repeat.exit_code == 0);
  T81_TEST_CHECK(repeat.stdout_text == valid.stdout_text);

  const auto read_decision = run_cli(
      t81_bin, {"artifact", "read-field", valid_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "decision"});
  T81_TEST_CHECK(read_decision.exit_code == 0);
  T81_TEST_CHECK(read_decision.stdout_text == "ALLOW\n");

  const auto read_action = run_cli(
      t81_bin, {"artifact", "read-field", valid_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "selected_action"});
  T81_TEST_CHECK(read_action.exit_code == 0);
  T81_TEST_CHECK(read_action.stdout_text == "write_allow_marker\n");

  const auto repeat_read = run_cli(
      t81_bin, {"artifact", "read-field", valid_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "selected_action"});
  T81_TEST_CHECK(repeat_read.exit_code == 0);
  T81_TEST_CHECK(repeat_read.stdout_text == read_action.stdout_text);

  const auto missing_writer_field = run_cli(
      t81_bin, {"artifact", "write-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--out",
                make_temp_path("t81-artifact-missing", ".json").string(), "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field", "decision=ALLOW", "--field",
                "termination_reason=single_step_max_score", "--field",
                "selected_action=write_allow_marker", "--field",
                "selected_path=actions/allow.marker", "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333"});
  T81_TEST_CHECK(missing_writer_field.exit_code != 0);
  T81_TEST_CHECK(contains(missing_writer_field.stderr_text, "missing required field \"reason_code\""));

  const auto unknown_writer_field = run_cli(
      t81_bin, {"artifact", "write-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--out",
                make_temp_path("t81-artifact-unknown", ".json").string(), "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field", "decision=ALLOW", "--field", "reason_code=GREETING_PAIR", "--field",
                "termination_reason=single_step_max_score", "--field",
                "selected_action=write_allow_marker", "--field",
                "selected_path=actions/allow.marker", "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--field", "extra_field=unexpected"});
  T81_TEST_CHECK(unknown_writer_field.exit_code != 0);
  T81_TEST_CHECK(contains(unknown_writer_field.stderr_text, "unknown field \"extra_field\""));

  const auto wrong_writer_schema = run_cli(
      t81_bin, {"artifact", "write-record", "--schema", "t81.ai.task.answer-fixed.v1", "--out",
                make_temp_path("t81-artifact-wrong-schema", ".json").string(), "--field",
                "decision=ALLOW"});
  T81_TEST_CHECK(wrong_writer_schema.exit_code != 0);
  T81_TEST_CHECK(contains(wrong_writer_schema.stderr_text,
                          "supports only schema t81.ai.task.assess-fixed.host-action-record.v1 or "
                          "t81.ai.task.route-fixed.path-selection-record.v1"));

  const auto put_ref =
      run_cli(t81_bin, {"canonfs", "put-file", valid_record.string(), "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(put_ref.exit_code == 0);
  std::string ref_text = put_ref.stdout_text;
  while (!ref_text.empty() && (ref_text.back() == '\n' || ref_text.back() == '\r')) {
    ref_text.pop_back();
  }
  const auto by_ref = run_cli(
      t81_bin, {"artifact", "validate-record", ref_text, "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(by_ref.exit_code == 0);
  T81_TEST_CHECK(contains(by_ref.stdout_text, "\"selector_kind\": \"canonfs_ref\""));

  const auto read_ref_field = run_cli(
      t81_bin, {"artifact", "read-field", ref_text, "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "action_ref",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(read_ref_field.exit_code == 0);
  T81_TEST_CHECK(read_ref_field.stdout_text ==
                 "sha3-256:333333333333333333333333333333333333333333333333333333333\n");

  const auto store_valid = run_cli(
      t81_bin, {"artifact", "store-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--file", valid_record.string(),
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(store_valid.exit_code == 0);
  T81_TEST_CHECK(contains(store_valid.stdout_text, "\"schema\": \"t81.artifact.record-store.v1\""));
  T81_TEST_CHECK(contains(store_valid.stdout_text, "\"validation_result\": \"pass\""));
  const auto extract_record_ref = [&](const std::string& text) {
    const std::string needle = "\"record_ref\": \"";
    const std::size_t start = text.find(needle);
    if (start == std::string::npos) {
      return std::string{};
    }
    const std::size_t value_start = start + needle.size();
    const std::size_t value_end = text.find('"', value_start);
    if (value_end == std::string::npos) {
      return std::string{};
    }
    return text.substr(value_start, value_end - value_start);
  };
  const std::string stored_ref = extract_record_ref(store_valid.stdout_text);
  T81_TEST_CHECK(!stored_ref.empty());

  const auto store_repeat = run_cli(
      t81_bin, {"artifact", "store-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--file", valid_record.string(),
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(store_repeat.exit_code == 0);
  T81_TEST_CHECK(extract_record_ref(store_repeat.stdout_text) == stored_ref);

  const auto write_store_valid = run_cli(
      t81_bin, {"artifact", "write-store-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field", "decision=ALLOW", "--field", "reason_code=GREETING_PAIR", "--field",
                "termination_reason=single_step_max_score", "--field",
                "selected_action=write_allow_marker", "--field",
                "selected_path=actions/allow.marker", "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(write_store_valid.exit_code == 0);
  T81_TEST_CHECK(
      contains(write_store_valid.stdout_text, "\"schema\": \"t81.artifact.record-write-store.v1\""));
  T81_TEST_CHECK(contains(write_store_valid.stdout_text, "\"validation_result\": \"pass\""));
  const std::string write_store_ref = extract_record_ref(write_store_valid.stdout_text);
  T81_TEST_CHECK(!write_store_ref.empty());
  T81_TEST_CHECK(write_store_ref == stored_ref);

  const auto write_store_repeat = run_cli(
      t81_bin, {"artifact", "write-store-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field", "decision=ALLOW", "--field", "reason_code=GREETING_PAIR", "--field",
                "termination_reason=single_step_max_score", "--field",
                "selected_action=write_allow_marker", "--field",
                "selected_path=actions/allow.marker", "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(write_store_repeat.exit_code == 0);
  T81_TEST_CHECK(extract_record_ref(write_store_repeat.stdout_text) == write_store_ref);

  const auto stored_readback = run_cli(
      t81_bin, {"artifact", "read-field", stored_ref, "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "selected_action",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(stored_readback.exit_code == 0);
  T81_TEST_CHECK(stored_readback.stdout_text == "write_allow_marker\n");

  const auto write_store_readback = run_cli(
      t81_bin, {"artifact", "read-field", write_store_ref, "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "action_ref",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(write_store_readback.exit_code == 0);
  T81_TEST_CHECK(write_store_readback.stdout_text ==
                 "sha3-256:333333333333333333333333333333333333333333333333333333333\n");

  const auto bundle_store = run_cli(
      t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.assess-fixed.bundle.v1",
                "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--field", "record_ref=" + stored_ref, "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(bundle_store.exit_code == 0);
  T81_TEST_CHECK(contains(bundle_store.stdout_text, "\"schema\": \"t81.artifact.bundle-store.v1\""));
  T81_TEST_CHECK(contains(bundle_store.stdout_text, "\"validation_result\": \"pass\""));
  const auto extract_bundle_ref = [&](const std::string& text) {
    const std::string needle = "\"bundle_ref\": \"";
    const std::size_t start = text.find(needle);
    if (start == std::string::npos) {
      return std::string{};
    }
    const std::size_t value_start = start + needle.size();
    const std::size_t value_end = text.find('"', value_start);
    if (value_end == std::string::npos) {
      return std::string{};
    }
    return text.substr(value_start, value_end - value_start);
  };
  const std::string bundle_ref = extract_bundle_ref(bundle_store.stdout_text);
  T81_TEST_CHECK(!bundle_ref.empty());

  const auto bundle_repeat = run_cli(
      t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.assess-fixed.bundle.v1",
                "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--field", "record_ref=" + stored_ref, "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(bundle_repeat.exit_code == 0);
  T81_TEST_CHECK(extract_bundle_ref(bundle_repeat.stdout_text) == bundle_ref);

  const auto bundle_readback = run_cli(
      t81_bin, {"artifact", "read-field", bundle_ref, "--schema",
                "t81.ai.task.assess-fixed.bundle.v1", "--field", "record_ref",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(bundle_readback.exit_code == 0);
  T81_TEST_CHECK(bundle_readback.stdout_text == stored_ref + "\n");

  const auto route_write_store = run_cli(
      t81_bin, {"artifact", "write-store-record", "--schema",
                "t81.ai.task.route-fixed.path-selection-record.v1", "--field",
                "source_result_ref=sha3-256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                "--field",
                "source_provenance_ref=sha3-256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
                "--field", "route=A", "--field", "termination_reason=single_step_max_score",
                "--field", "selected_action=write_route_a_target", "--field",
                "selected_path=routes/a.target", "--field",
                "action_ref=sha3-256:ccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(route_write_store.exit_code == 0);
  const std::string route_record_ref = extract_record_ref(route_write_store.stdout_text);
  T81_TEST_CHECK(!route_record_ref.empty());

  const auto route_record_read = run_cli(
      t81_bin, {"artifact", "read-field", route_record_ref, "--schema",
                "t81.ai.task.route-fixed.path-selection-record.v1", "--field", "selected_path",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(route_record_read.exit_code == 0);
  T81_TEST_CHECK(route_record_read.stdout_text == "routes/a.target\n");

  const auto route_bundle_store = run_cli(
      t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.route-fixed.bundle.v1",
                "--field",
                "source_result_ref=sha3-256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                "--field",
                "source_provenance_ref=sha3-256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
                "--field",
                "action_ref=sha3-256:ccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
                "--field", "record_ref=" + route_record_ref, "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(route_bundle_store.exit_code == 0);
  const std::string route_bundle_ref = extract_bundle_ref(route_bundle_store.stdout_text);
  T81_TEST_CHECK(!route_bundle_ref.empty());

  const auto route_bundle_read = run_cli(
      t81_bin, {"artifact", "read-field", route_bundle_ref, "--schema",
                "t81.ai.task.route-fixed.bundle.v1", "--field", "record_ref",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(route_bundle_read.exit_code == 0);
  T81_TEST_CHECK(route_bundle_read.stdout_text == route_record_ref + "\n");

  const auto missing = run_cli(
      t81_bin, {"artifact", "validate-record", missing_field_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1"});
  T81_TEST_CHECK(missing.exit_code != 0);
  T81_TEST_CHECK(contains(missing.stderr_text, "missing required field \"reason_code\""));

  const auto wrong = run_cli(
      t81_bin, {"artifact", "validate-record", wrong_schema_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1"});
  T81_TEST_CHECK(wrong.exit_code != 0);
  T81_TEST_CHECK(contains(wrong.stderr_text, "schema mismatch"));

  const auto missing_read = run_cli(
      t81_bin, {"artifact", "read-field", missing_field_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "reason_code"});
  T81_TEST_CHECK(missing_read.exit_code != 0);
  T81_TEST_CHECK(contains(missing_read.stderr_text, "missing required field \"reason_code\""));

  const auto wrong_read = run_cli(
      t81_bin, {"artifact", "read-field", wrong_schema_record.string(), "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field", "decision"});
  T81_TEST_CHECK(wrong_read.exit_code != 0);
  T81_TEST_CHECK(contains(wrong_read.stderr_text, "schema mismatch"));

  const auto invalid_store = run_cli(
      t81_bin, {"artifact", "store-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--file",
                missing_field_record.string(), "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(invalid_store.exit_code != 0);
  T81_TEST_CHECK(contains(invalid_store.stderr_text, "missing required field \"reason_code\""));

  const auto wrong_store = run_cli(
      t81_bin, {"artifact", "store-record", "--schema",
                "t81.ai.task.answer-fixed.v1", "--file", valid_record.string(), "--canonfs-root",
                canonfs_root.string()});
  T81_TEST_CHECK(wrong_store.exit_code != 0);
  T81_TEST_CHECK(contains(wrong_store.stderr_text, "schema mismatch"));

  const auto missing_write_store = run_cli(
      t81_bin, {"artifact", "write-store-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field", "decision=ALLOW", "--field",
                "termination_reason=single_step_max_score", "--field",
                "selected_action=write_allow_marker", "--field",
                "selected_path=actions/allow.marker", "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(missing_write_store.exit_code != 0);
  T81_TEST_CHECK(contains(missing_write_store.stderr_text, "missing required field \"reason_code\""));

  const auto unknown_write_store = run_cli(
      t81_bin, {"artifact", "write-store-record", "--schema",
                "t81.ai.task.assess-fixed.host-action-record.v1", "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field", "decision=ALLOW", "--field", "reason_code=GREETING_PAIR", "--field",
                "termination_reason=single_step_max_score", "--field",
                "selected_action=write_allow_marker", "--field",
                "selected_path=actions/allow.marker", "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--field", "extra_field=unexpected", "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(unknown_write_store.exit_code != 0);
  T81_TEST_CHECK(contains(unknown_write_store.stderr_text, "unknown field \"extra_field\""));

  const auto wrong_write_store = run_cli(
      t81_bin, {"artifact", "write-store-record", "--schema", "t81.ai.task.answer-fixed.v1",
                "--field", "decision=ALLOW", "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(wrong_write_store.exit_code != 0);
  T81_TEST_CHECK(contains(wrong_write_store.stderr_text,
                          "supports only schema t81.ai.task.assess-fixed.host-action-record.v1 or "
                          "t81.ai.task.route-fixed.path-selection-record.v1"));

  const auto missing_bundle = run_cli(
      t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.assess-fixed.bundle.v1",
                "--field",
                "source_result_ref=sha3-256:111111111111111111111111111111111111111111111111111111111",
                "--field",
                "source_provenance_ref=sha3-256:222222222222222222222222222222222222222222222222222222222",
                "--field",
                "action_ref=sha3-256:333333333333333333333333333333333333333333333333333333333",
                "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(missing_bundle.exit_code != 0);
  T81_TEST_CHECK(contains(missing_bundle.stderr_text, "missing required field \"record_ref\""));

  const auto wrong_bundle = run_cli(
      t81_bin, {"artifact", "store-bundle", "--schema", "t81.ai.task.answer-fixed.v1",
                "--field", "record_ref=" + stored_ref, "--canonfs-root", canonfs_root.string()});
  T81_TEST_CHECK(wrong_bundle.exit_code != 0);
  T81_TEST_CHECK(contains(wrong_bundle.stderr_text,
                          "supports only schema t81.ai.task.assess-fixed.bundle.v1 or "
                          "t81.ai.task.route-fixed.bundle.v1"));

  return 0;
}
