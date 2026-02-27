#include "test_runtime_check.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr const char* kExpectedVersion = "1.1.0";

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

struct CommandResult {
  int exit_code = -1;
  std::string stdout_text;
  std::string stderr_text;
};

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

  std::string cmd = shell_escape(bin_path.string());
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

  std::error_code ignore_ec;
  fs::remove(out_path, ignore_ec);
  fs::remove(err_path, ignore_ec);
  fs::remove(code_path, ignore_ec);
  return result;
}

bool contains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

}  // namespace

int main(int argc, char* argv[]) {
  T81_TEST_CHECK(argc >= 2);
  const fs::path t81_bin = fs::path(argv[1]);
  T81_TEST_CHECK(fs::exists(t81_bin));

  {
    const auto result = run_cli(t81_bin, {"help", "compile"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 compile"));
  }

  {
    const auto result = run_cli(t81_bin, {"--help"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "code    <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "project <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "env     <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "internal <action> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "completion <shell>"));
    T81_TEST_CHECK(contains(result.stderr_text, "man [--install-dir <dir>]"));
    T81_TEST_CHECK(contains(result.stderr_text, "feedback <subcommand> [args]"));
    T81_TEST_CHECK(contains(result.stderr_text, "compile <file.t81|.t81w>"));
    T81_TEST_CHECK(contains(result.stderr_text, "test    [options] [-- ...]"));
    T81_TEST_CHECK(contains(result.stderr_text, "doctor  [--json]"));
    T81_TEST_CHECK(contains(result.stderr_text, "fmt     [options] <file...>"));
    T81_TEST_CHECK(!contains(result.stderr_text, "pkg     <subcommand>"));
    T81_TEST_CHECK(!contains(result.stderr_text, "llama-run <model.gguf>"));
    T81_TEST_CHECK(!contains(result.stderr_text, "weights <subcommand>"));
    T81_TEST_CHECK(contains(result.stderr_text, "t81 help advanced"));
    T81_TEST_CHECK(contains(result.stderr_text, "t81 help labs"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "code"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 code <action>"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "code", "build"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 compile"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "completion"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 completion"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "man"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 man"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "feedback"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 feedback"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "advanced"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "weights <subcommand>"));
    T81_TEST_CHECK(contains(result.stderr_text, "policy <subcommand>"));
    T81_TEST_CHECK(contains(result.stderr_text, "trace <subcommand>"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "labs"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "pkg <subcommand>"));
    T81_TEST_CHECK(contains(result.stderr_text, "benchmark"));
    T81_TEST_CHECK(contains(result.stderr_text, "llama-run"));
  }

  {
    const auto result = run_cli(t81_bin, {"compile", "--help"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 compile"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "weights", "info"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 weights info"));
  }

  {
    const auto result = run_cli(t81_bin, {"weights", "info", "--help"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 weights info"));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "test"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Usage: t81 test"));
  }

  {
    const auto result = run_cli(t81_bin, {"doctor", "--json"});
    T81_TEST_CHECK(result.exit_code == 0 || result.exit_code == 2);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.doctor.v1\""));
    T81_TEST_CHECK(contains(result.stdout_text, "\"checks\""));
  }

  {
    const auto result = run_cli(t81_bin, {"fmt", "--version"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, std::string("t81-fmt ") + kExpectedVersion));
  }

  {
    const auto result = run_cli(t81_bin, {"help", "nope"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Unknown help topic"));
  }

  {
    const auto result = run_cli(t81_bin, {"-q", "version"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, std::string("T81 Foundation ") + kExpectedVersion));
  }

  {
    const auto result = run_cli(t81_bin, {"version", "-q"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, std::string("T81 Foundation ") + kExpectedVersion));
  }

  {
    const auto result = run_cli(t81_bin, {"-q", "nope"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Unknown command"));
  }

  {
    const auto result = run_cli(t81_bin, {"init", "bad name"});
    T81_TEST_CHECK(result.exit_code != 0);
    T81_TEST_CHECK(contains(result.stderr_text, "Project name must contain only"));
  }

  {
    const fs::path temp_dir = fs::temp_directory_path() / "t81-cli-contract-pkg";
    std::error_code ignore_ec;
    fs::remove_all(temp_dir, ignore_ec);
    fs::create_directories(temp_dir);
    const fs::path old_cwd = fs::current_path();
    fs::current_path(temp_dir);

    {
      std::ofstream out("package.t81");
      out << "(package\n"
             "  (name \"ok_pkg\")\n"
             "  (version \"1.2.3\")\n"
             ")\n";
    }
    const auto ok_result = run_cli(t81_bin, {"pkg", "check", "--json"});
    T81_TEST_CHECK(ok_result.exit_code == 0);
    T81_TEST_CHECK(contains(ok_result.stdout_text, "\"schema\": \"t81.pkg-check.v1\""));
    T81_TEST_CHECK(contains(ok_result.stdout_text, "\"valid\": true"));

    {
      std::ofstream out("bad.t81");
      out << "(package\n"
             "  (name \"bad pkg\")\n"
             "  (version \"x.y.z\")\n"
             ")\n";
    }
    const auto bad_result = run_cli(t81_bin, {"pkg", "check", "bad.t81", "--json"});
    T81_TEST_CHECK(bad_result.exit_code == 2);
    T81_TEST_CHECK(contains(bad_result.stdout_text, "\"valid\": false"));

    fs::current_path(old_cwd);
    fs::remove_all(temp_dir, ignore_ec);
  }

  {
    const auto result = run_cli(
        t81_bin, {"test", "--json", "--list", "--build-dir", t81_bin.parent_path().string()});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.test.v1\""));
    T81_TEST_CHECK(contains(result.stdout_text, "\"summary\""));
    T81_TEST_CHECK(contains(result.stdout_text, "\"summary_source\""));
  }

  {
    const auto result = run_cli(t81_bin, {"test", "--build-dir", "definitely_missing_build_dir"});
    T81_TEST_CHECK(result.exit_code == 2);
    T81_TEST_CHECK(contains(result.stderr_text, "Run cmake configure/build first"));
  }

  {
    const fs::path temp_dir = fs::temp_directory_path() / "t81-cli-contract-fmt";
    std::error_code ignore_ec;
    fs::remove_all(temp_dir, ignore_ec);
    fs::create_directories(temp_dir);
    const fs::path file = temp_dir / "format_me.t81";
    {
      std::ofstream out(file);
      out << "fn main() -> i32 {\n"
             "print(\"x\");   \n"
             "return 0;\n"
             "}\n";
    }

    const auto check_result = run_cli(t81_bin, {"fmt", "--check", file.string()});
    T81_TEST_CHECK(check_result.exit_code == 2);
    T81_TEST_CHECK(contains(check_result.stdout_text, "would format"));

    const auto json_result = run_cli(t81_bin, {"fmt", "--json", file.string()});
    T81_TEST_CHECK(json_result.exit_code == 0);
    T81_TEST_CHECK(contains(json_result.stdout_text, "\"schema\": \"t81.fmt.v1\""));
    const std::string expected_fmt_ver =
        std::string("\"formatter_version\": \"t81-fmt ") + kExpectedVersion + "\"";
    T81_TEST_CHECK(contains(json_result.stdout_text, expected_fmt_ver));

    const fs::path bad_file = temp_dir / "invalid.t81";
    {
      std::ofstream out(bad_file);
      out << "fn main() -> i32 {\n"
             "  let x = ;\n"
             "}\n";
    }
    const auto bad_result = run_cli(t81_bin, {"fmt", bad_file.string()});
    T81_TEST_CHECK(bad_result.exit_code == 1);
    T81_TEST_CHECK(contains(bad_result.stderr_text, "Invalid .t81 input (cannot safely format)"));

    fs::remove_all(temp_dir, ignore_ec);
  }

  {
    const auto result = run_cli(t81_bin, {"weights", "info"});
    T81_TEST_CHECK(result.exit_code == 1);
    T81_TEST_CHECK(contains(result.stderr_text, "Run 't81 help weights info'"));
  }

  {
    const auto result = run_cli(t81_bin, {"weights", "info", "does-not-exist.t81w", "--json"});
    T81_TEST_CHECK(result.exit_code == 1);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.weights-info.v1\""));
    T81_TEST_CHECK(contains(result.stdout_text, "\"ok\": false"));
  }

  {
    const fs::path policy = t81_bin.parent_path().parent_path() / "examples/system_integration.apl";
    T81_TEST_CHECK(fs::exists(policy));
    const auto result = run_cli(t81_bin, {"policy", "run", policy.string(), "--json"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\": \"t81.policy-run.v1\""));
    const auto bad_result = run_cli(t81_bin, {"policy", "run", policy.string(), "--bad-flag"});
    T81_TEST_CHECK(bad_result.exit_code == 1);
    T81_TEST_CHECK(contains(bad_result.stderr_text, "Run 't81 help policy run'"));
  }

  {
    const fs::path temp_dir = fs::temp_directory_path() / "t81-cli-contract-trace-export";
    std::error_code ignore_ec;
    fs::remove_all(temp_dir, ignore_ec);
    fs::create_directories(temp_dir);
    const fs::path trace = temp_dir / "a.trace";
    {
      std::ofstream out(trace);
      out << "PC=0 NOP\n";
    }
    const auto result = run_cli(t81_bin, {"trace", "export", trace.string(), "--format", "json"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "\"schema\":\"t81.trace-export-entry.v1\""));
    const auto bad_format =
        run_cli(t81_bin, {"trace", "export", trace.string(), "--format", "xml"});
    T81_TEST_CHECK(bad_format.exit_code == 1);
    fs::remove_all(temp_dir, ignore_ec);
  }

  {
    const auto result = run_cli(t81_bin, {"completion", "bash"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "complete -F _t81_complete t81"));
  }

  {
    const auto result = run_cli(t81_bin, {"completion", "wat"});
    T81_TEST_CHECK(result.exit_code == 1);
  }

  {
    const auto result = run_cli(t81_bin, {"man"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, ".TH T81 1"));
  }

  {
    const fs::path temp_dir = fs::temp_directory_path() / "t81-cli-contract-feedback";
    std::error_code ignore_ec;
    fs::remove_all(temp_dir, ignore_ec);
    fs::create_directories(temp_dir);
    const fs::path feedback_path = temp_dir / "feedback.jsonl";
    const auto submit = run_cli(t81_bin, {"feedback", "submit", "--rating", "4", "--note", "good",
                                          "--path", feedback_path.string()});
    T81_TEST_CHECK(submit.exit_code == 0);
    const auto report = run_cli(t81_bin, {"feedback", "report", "--path", feedback_path.string()});
    T81_TEST_CHECK(report.exit_code == 0);
    T81_TEST_CHECK(contains(report.stdout_text, "\"schema\": \"t81.feedback-report.v1\""));
    fs::remove_all(temp_dir, ignore_ec);
  }

  {
    const fs::path temp_dir = fs::temp_directory_path() / "t81-cli-contract";
    std::error_code ignore_ec;
    fs::remove_all(temp_dir, ignore_ec);
    fs::create_directories(temp_dir);

    const fs::path source_path = temp_dir / "hello.t81";
    const fs::path program_path = temp_dir / "hello.tisc";
    {
      std::ofstream out(source_path);
      out << "fn main() -> i32 {\n"
             "  print(\"hello-cli-contract\");\n"
             "  return 0;\n"
             "}\n";
    }

    const auto compile_result =
        run_cli(t81_bin, {"code", "build", source_path.string(), "-o", program_path.string()});
    T81_TEST_CHECK(compile_result.exit_code == 0);

    const fs::path legacy_program_path = temp_dir / "legacy.tisc";
    const auto legacy_compile_result =
        run_cli(t81_bin, {"compile", source_path.string(), "-o", legacy_program_path.string()});
    T81_TEST_CHECK(legacy_compile_result.exit_code == 0);
    T81_TEST_CHECK(contains(legacy_compile_result.stderr_text, "legacy alias"));
    T81_TEST_CHECK(contains(legacy_compile_result.stderr_text, "t81 code build"));

    const auto run_result = run_cli(t81_bin, {"code", "run", program_path.string()});
    T81_TEST_CHECK(run_result.exit_code == 0);
    T81_TEST_CHECK(contains(run_result.stdout_text, "hello-cli-contract"));
    T81_TEST_CHECK(!contains(run_result.stderr_text, "hello-cli-contract"));

    fs::remove_all(temp_dir, ignore_ec);
  }

  {
    const fs::path temp_dir = fs::temp_directory_path() / "t81-cli-contract-trace";
    std::error_code ignore_ec;
    fs::remove_all(temp_dir, ignore_ec);
    fs::create_directories(temp_dir);
    const fs::path trace_a = temp_dir / "a.trace";
    const fs::path trace_b = temp_dir / "b.trace";
    {
      std::ofstream out(trace_a);
      out << "PC=0 NOP\n";
    }
    {
      std::ofstream out(trace_b);
      out << "PC=0 HALT\n";
    }
    const auto result =
        run_cli(t81_bin, {"trace", "diff", trace_a.string(), trace_b.string(), "--no-color"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stdout_text, "Difference at line 1"));
    T81_TEST_CHECK(!contains(result.stdout_text, "\x1b["));
    fs::remove_all(temp_dir, ignore_ec);
  }

  return 0;
}
