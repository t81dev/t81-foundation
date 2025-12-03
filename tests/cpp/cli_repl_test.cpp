#include "t81/cli/driver.hpp"
#include "t81/weights.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>

namespace {
class StreamCapture {
 public:
  StreamCapture(std::ostream& target, std::streambuf* new_buf)
      : stream_(target), old_buf_(stream_.rdbuf(new_buf)) {}

  ~StreamCapture() { stream_.rdbuf(old_buf_); }

 private:
  std::ostream& stream_;
  std::streambuf* old_buf_;
};

struct ReplResult {
  int rc = 0;
  std::string output;
  std::string errors;
};

ReplResult run_repl_script(const std::string& script,
                          const std::shared_ptr<t81::weights::ModelFile>& weights_model) {
  std::istringstream input(script);
  std::ostringstream output;
  std::ostringstream errors;
  StreamCapture stdout_capture(std::cout, output.rdbuf());
  StreamCapture stderr_capture(std::cerr, errors.rdbuf());

  int rc = t81::cli::repl(weights_model, input);
  return {rc, output.str(), errors.str()};
}
}  // namespace

namespace fs = std::filesystem;

int main() {
  const fs::path load_path = fs::temp_directory_path() / "t81_repl_load.t81";
  const fs::path save_path = fs::temp_directory_path() / "t81_repl_saved.t81";
  const fs::path missing_model = fs::temp_directory_path() / "t81_repl_missing.model";

  {
    std::ofstream load_file(load_path);
    load_file << R"(fn helper() -> i32 {
    return 42;
}
)";
  }
  std::error_code ignore_ec;
  fs::remove(save_path, ignore_ec);

  std::ostringstream script;
  script << R"(fn main() -> i32 {
    return 123;
}

)";
  script << ":history\n";
  script << ":reset\n";
  script << ":history\n";
  script << ":load " << load_path << '\n';
  script << ":history\n";
  script << ":save " << save_path << '\n';
  script << ":run\n";
  script << ":bindings\n";
  script << ":trace\n";
  script << ":model\n";
  script << ":model " << missing_model << '\n';
  script << ":model none\n";
  script << ":load " << save_path << '\n';
  script << ":quiet\n";
  script << ":verbose\n";
  script << ":run\n";
  script << ":quit\n";

  auto run_and_assert = [&](const std::shared_ptr<t81::weights::ModelFile>& weights_model) {
    ReplResult result = run_repl_script(script.str(), weights_model);
    assert(result.rc == 0);
    assert(result.output.find("REPL buffer") != std::string::npos);
    assert(result.output.find("REPL buffer cleared") != std::string::npos);
    assert(result.output.find("Loaded snippet from") != std::string::npos);
    assert(result.output.find("Buffer saved to") != std::string::npos);
    assert(result.output.find("Execution completed") != std::string::npos);
    assert(result.output.find("Symbols from last run") != std::string::npos);
    assert(result.output.find("Last trace entries:") != std::string::npos);
    assert(result.output.find("No weights model attached") != std::string::npos);
    assert(result.output.find("Weights model cleared") != std::string::npos);
    assert(result.output.find("Quiet mode enabled") != std::string::npos);
    assert(result.output.find("Verbose logging enabled") != std::string::npos);
    assert(result.errors.find("Failed to load model") != std::string::npos);
  };

  run_and_assert(nullptr);
  auto weights = std::make_shared<t81::weights::ModelFile>();
  run_and_assert(weights);

  {
    std::ifstream saved(save_path);
    assert(saved);
    std::string contents((std::istreambuf_iterator<char>(saved)), std::istreambuf_iterator<char>());
    assert(contents.find("fn helper()") != std::string::npos);
  }

  fs::remove(load_path, ignore_ec);
  fs::remove(save_path, ignore_ec);
  return 0;
}
