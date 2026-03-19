#include "t81/axion/shell/shell_session.hpp"
#include "t81/axion/userenv/service_registry.hpp"
#include "t81/axion/userenv/session_manager.hpp"
#include "t81/axion/userenv/t81sh.hpp"

int main() {
  using namespace t81::ternaryos;
  using namespace t81::ternaryos::userenv;

  ServiceEntry service;
  service.id = "svc";
  service.binary = "/bin/svc";
  service.canon_hash = "deadbeef";

  ServiceRegistry registry = make_service_registry({service});
  SessionManager mgr;
  mgr.load_principals({});

  SessionRecord session;
  session.session_id = 1;
  session.principal_name = "root";
  T81Shell shell(session, 1);

  (void)registry;
  (void)shell;
  return default_shell_command_sequence().empty() ? 1 : 0;
}
