#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "vm_canonfs_root_env_contract_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

void set_env(const std::filesystem::path& value) {
#if defined(_WIN32)
  _putenv_s("T81_CANONFS_ROOT", value.string().c_str());
#else
  setenv("T81_CANONFS_ROOT", value.string().c_str(), 1);
#endif
}

void unset_env() {
#if defined(_WIN32)
  _putenv_s("T81_CANONFS_ROOT", "");
#else
  unsetenv("T81_CANONFS_ROOT");
#endif
}

}  // namespace

int main() {
  namespace fs = std::filesystem;

  const fs::path original_cwd = fs::current_path();
  const fs::path sandbox = fs::temp_directory_path() / "t81-vm-canonfs-root-env-contract";
  const fs::path cwd_root = sandbox / "cwd";
  const fs::path env_root = sandbox / "env-store";

  std::error_code ec;
  fs::remove_all(sandbox, ec);
  fs::create_directories(cwd_root, ec);
  if (!expect(!ec, "failed to create sandbox directories")) return 1;

  fs::current_path(cwd_root, ec);
  if (!expect(!ec, "failed to switch cwd")) return 1;

  set_env(env_root);
  {
    auto vm = t81::vm::make_interpreter_vm();
    if (!expect(static_cast<bool>(vm), "failed to construct VM")) return 1;
  }

  const fs::path implicit_root = cwd_root / ".t81_canonfs";
  if (!expect(fs::exists(env_root), "VM did not create CanonFS root from environment")) return 1;
  if (!expect(!fs::exists(implicit_root), "VM should not create cwd CanonFS root when env override is set")) {
    return 1;
  }

  unset_env();
  fs::current_path(original_cwd, ec);
  fs::remove_all(sandbox, ec);
  return 0;
}
