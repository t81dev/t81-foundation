#include "t81/cli/driver.hpp"
#include "t81/weights.hpp"

#include <cassert>
#include <iostream>
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

int run_repl_script(const std::string& script,
                    const std::shared_ptr<t81::weights::ModelFile>& weights_model) {
  std::istringstream input(script);
  std::ostringstream output;
  std::ostringstream errors;
  StreamCapture stdout_capture(std::cout, output.rdbuf());
  StreamCapture stderr_capture(std::cerr, errors.rdbuf());

  int rc = t81::cli::repl(weights_model, input);

  const std::string captured_output = output.str();
  const std::string captured_errors = errors.str();
  assert(rc == 0);
  assert(captured_errors.empty());
  assert(captured_output.find("Execution completed") != std::string::npos);
  return rc;
}
}  // namespace

int main() {
  const std::string script = R"(fn main() -> i32 {
    return 123;
}

:quit
)";

  run_repl_script(script, nullptr);
  auto weights = std::make_shared<t81::weights::ModelFile>();
  run_repl_script(script, weights);
  return 0;
}
