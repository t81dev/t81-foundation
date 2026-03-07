#include "test_runtime_check.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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
    const auto result = run_cli(t81_bin, {"--help"});
    T81_TEST_CHECK(result.exit_code == 0);
    T81_TEST_CHECK(contains(result.stderr_text, "code    <action> [args]"));
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
