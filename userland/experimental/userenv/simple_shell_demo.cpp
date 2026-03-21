// experimental/ternaryos/userenv/simple_shell_demo.cpp
//
// RFC-00B9 §8 — Simple t81sh demonstration without complex features.

#include "t81sh.hpp"

#include <cinttypes>
#include <cstdio>

namespace t81::ternaryos::userenv {

static SessionRecord make_demo_session() {
  SessionRecord session;
  session.session_id = 1;
  session.principal_id = 2;
  session.principal_name = "user";
  session.tty_handle = "/dev/tty0";
  session.root_pgid = 1000;
  session.start_epoch = 12345;
  session.state = SessionState::Active;
  session.canon_hash = "33fdc7dab8736cec";
  return session;
}

static void demo_basic_shell() {
  std::printf("=== Basic Shell Demo ===\n");
  
  auto session = make_demo_session();
  T81Shell shell(session, 1);
  
  // Test prompt
  std::string prompt = shell.prompt();
  std::printf("Prompt: '%s'\n", prompt.c_str());
  
  // Test simple builtin
  bool result = shell.exec_command("help", 1000);
  std::printf("Help command: %s\n", result ? "Success" : "Failed");
  
  // Test history
  const auto& history = shell.history();
  std::printf("History entries: %zu\n", history.size());
  
  // Test CanonFS path
  std::string history_path = shell.history_canon_path();
  std::printf("History path: %s\n", history_path.c_str());
}

} // namespace t81::ternaryos::userenv

int main() {
  std::printf("Simple T81 Shell Demo\n");
  std::printf("=====================\n");
  
  t81::ternaryos::userenv::demo_basic_shell();
  
  return 0;
}
