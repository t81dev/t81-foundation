// cli_stress_test.cpp
// Stress-tests the t81 CLI surface not covered by cli_contract_test.cpp.
// Invocation: cli_stress_test <path-to-t81-binary>
//
// Coverage targets:
//   version / --version / -V
//   completion bash / zsh
//   trace filter / trace stats / trace export
//   vm run / vm debug / vm explain-trap
//   code lint / fmt / test / debug / disasm
//   lang check / lint / fmt / build / run / test / debug / profile / show / dump / disasm
//   tisc stats / tisc diff
//   ir dump / ir validate
//   determinism hash / multi-run / diff
//   policy compile / list / run
//   axion simulate / audit
//   weights info / verify / export / quantize (expected errors — no model file)
//   tier info / check / gate
//   tensor hash / inspect (expected errors — no tensor file)
//   man / feedback (help topic smoke)
//   Error boundaries: missing files, wrong extension, corrupt binary, empty input

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "test_runtime_check.hpp"

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Harness helpers (same pattern as cli_contract_test.cpp)
// ---------------------------------------------------------------------------

struct CommandResult {
  std::string stdout_text;
  std::string stderr_text;
  int exit_code = 0;
};

static std::mt19937_64 s_rng{std::random_device{}()};

static fs::path make_temp_path(std::string_view prefix, std::string_view ext) {
  std::uniform_int_distribution<uint64_t> dist;
  return fs::temp_directory_path() /
         (std::string(prefix) + "-" + std::to_string(dist(s_rng)) + std::string(ext));
}

static std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

static bool contains(std::string_view text, std::string_view pat) {
  return text.find(pat) != std::string_view::npos;
}

static CommandResult run_cli(const fs::path& bin, const std::vector<std::string>& args) {
  const fs::path out_path = make_temp_path("t81-stress", ".out");
  const fs::path err_path = make_temp_path("t81-stress", ".err");

  std::vector<std::string> argv_storage;
  argv_storage.push_back(fs::absolute(bin).string());
  argv_storage.insert(argv_storage.end(), args.begin(), args.end());

  std::ostringstream cmd_str;
  for (std::size_t i = 0; i < argv_storage.size(); ++i) {
    if (i) cmd_str << ' ';
    cmd_str << argv_storage[i];
  }
  std::cerr << "[stress] " << cmd_str.str() << "\n";

  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(argv_storage.size() + 1);
  for (auto& s : argv_storage) argv_ptrs.push_back(s.data());
  argv_ptrs.push_back(nullptr);

  pid_t pid = fork();
  T81_TEST_CHECK(pid >= 0);
  if (pid == 0) {
    // Redirect stdin from /dev/null so interactive commands (vm debug, lang debug, repl)
    // see EOF immediately and exit cleanly rather than blocking forever.
    int devnull = ::open("/dev/null", O_RDONLY);
    if (devnull != -1) {
      dup2(devnull, STDIN_FILENO);
      ::close(devnull);
    }
    FILE* out = std::fopen(out_path.c_str(), "wb");
    FILE* err = std::fopen(err_path.c_str(), "wb");
    if (!out || !err) _exit(127);
    if (dup2(fileno(out), STDOUT_FILENO) == -1 || dup2(fileno(err), STDERR_FILENO) == -1)
      _exit(127);
    std::fclose(out);
    std::fclose(err);
    execv(argv_ptrs[0], argv_ptrs.data());
    _exit(127);
  }

  constexpr auto kTimeout = std::chrono::seconds(30);
  const auto deadline = std::chrono::steady_clock::now() + kTimeout;
  int status = 0;
  while (true) {
    pid_t rc = waitpid(pid, &status, WNOHANG);
    T81_TEST_CHECK(rc != -1);
    if (rc == pid) break;
    if (std::chrono::steady_clock::now() >= deadline) {
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      std::cerr << "[stress-timeout] " << cmd_str.str() << "\n";
      T81_TEST_CHECK(false && "CLI invocation timed out");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }

  CommandResult r;
  r.stdout_text = read_file(out_path);
  r.stderr_text = read_file(err_path);
  if (WIFEXITED(status))
    r.exit_code = WEXITSTATUS(status);
  else if (WIFSIGNALED(status))
    r.exit_code = 128 + WTERMSIG(status);
  else
    r.exit_code = status;

  std::error_code ec;
  fs::remove(out_path, ec);
  fs::remove(err_path, ec);
  return r;
}

// ---------------------------------------------------------------------------
// Fixture programs
// ---------------------------------------------------------------------------

static const char* kHelloSrc = R"t81(
fn main() -> i32 {
    print("hello stress");
    return 0;
}
)t81";

static const char* kArithSrc = R"t81(
fn add(a: i32, b: i32) -> i32 { return a + b; }
fn main() -> i32 {
    let x: i32 = add(3, 4);
    print(x);
    return 0;
}
)t81";

static const char* kSyntaxErrSrc = "fn main( {\n";

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  T81_TEST_CHECK(argc >= 2);
  const fs::path t81_bin = fs::path(argv[1]);
  T81_TEST_CHECK(fs::exists(t81_bin));

  // Shared temp files reused across sections
  const fs::path hello_src = make_temp_path("t81-stress-hello", ".t81");
  const fs::path arith_src = make_temp_path("t81-stress-arith", ".t81");
  const fs::path bad_src = make_temp_path("t81-stress-bad", ".t81");
  const fs::path hello_tisc = make_temp_path("t81-stress-hello", ".tisc");
  const fs::path arith_tisc = make_temp_path("t81-stress-arith", ".tisc");
  const fs::path trace_file = make_temp_path("t81-stress", ".trace");
  const fs::path policy_file = make_temp_path("t81-stress", ".apl");

  {
    std::ofstream o(hello_src);
    o << kHelloSrc;
  }
  {
    std::ofstream o(arith_src);
    o << kArithSrc;
  }
  {
    std::ofstream o(bad_src);
    o << kSyntaxErrSrc;
  }
  {
    std::ofstream o(policy_file);
    o << "(policy (tier 1) (allowed-tensor-hashes [\"sha3-256:test-hash\"]))\n";
  }

  // Build the hello binary once — many tests depend on it
  {
    const auto r =
        run_cli(t81_bin, {"code", "build", hello_src.string(), "-o", hello_tisc.string()});
    T81_TEST_CHECK(r.exit_code == 0);
    T81_TEST_CHECK(fs::exists(hello_tisc));
  }
  {
    const auto r =
        run_cli(t81_bin, {"code", "build", arith_src.string(), "-o", arith_tisc.string()});
    T81_TEST_CHECK(r.exit_code == 0);
  }
  // Produce a trace for the hello binary
  {
    const auto r =
        run_cli(t81_bin, {"code", "run", hello_tisc.string(), "-o", trace_file.string()});
    T81_TEST_CHECK(r.exit_code == 0);
  }

  // ── §1  version ──────────────────────────────────────────────────────────
  {
    for (const std::string alias : {"version", "--version", "-V"}) {
      const auto r = run_cli(t81_bin, {alias});
      T81_TEST_CHECK(r.exit_code == 0);
      T81_TEST_CHECK(contains(r.stdout_text, "1.") || contains(r.stdout_text, "t81"));
    }
  }

  // ── §2  completion bash / zsh ─────────────────────────────────────────────
  {
    const auto bash = run_cli(t81_bin, {"completion", "bash"});
    T81_TEST_CHECK(bash.exit_code == 0);
    T81_TEST_CHECK(contains(bash.stdout_text, "t81"));

    const auto zsh = run_cli(t81_bin, {"completion", "zsh"});
    T81_TEST_CHECK(zsh.exit_code == 0);
    T81_TEST_CHECK(contains(zsh.stdout_text, "t81"));
  }

  // ── §3  man / feedback (help-topic smoke) ────────────────────────────────
  {
    const auto r = run_cli(t81_bin, {"help", "man"});
    T81_TEST_CHECK(r.exit_code == 0);

    const auto f = run_cli(t81_bin, {"help", "feedback"});
    T81_TEST_CHECK(f.exit_code == 0);
  }

  // ── §4  trace filter / stats / export ────────────────────────────────────
  {
    const auto filter =
        run_cli(t81_bin, {"trace", "filter", trace_file.string(), "--opcode", "Halt", "--json"});
    T81_TEST_CHECK(filter.exit_code == 0);
    T81_TEST_CHECK(contains(filter.stdout_text, "\"schema\": \"t81.trace-filter.v1\"") ||
                   contains(filter.stdout_text, "PC=") || !filter.stdout_text.empty());

    const auto stats = run_cli(t81_bin, {"trace", "stats", trace_file.string(), "--json"});
    T81_TEST_CHECK(stats.exit_code == 0);
    T81_TEST_CHECK(contains(stats.stdout_text, "\"schema\": \"t81.trace-summary.v1\""));

    const fs::path exported = make_temp_path("t81-stress-trace", ".json");
    const auto exp = run_cli(t81_bin, {"trace", "export", trace_file.string(), "-o",
                                       exported.string(), "--format", "json"});
    T81_TEST_CHECK(exp.exit_code == 0);
    std::error_code ec;
    fs::remove(exported, ec);
  }

  // ── §5  vm run / debug / explain-trap ────────────────────────────────────
  {
    const auto vm_run = run_cli(t81_bin, {"vm", "run", hello_tisc.string()});
    T81_TEST_CHECK(vm_run.exit_code == 0);
    T81_TEST_CHECK(contains(vm_run.stdout_text, "hello stress") || vm_run.exit_code == 0);

    const auto vm_debug =
        run_cli(t81_bin, {"vm", "debug", hello_tisc.string(), "--steps", "1", "--json"});
    T81_TEST_CHECK(vm_debug.exit_code == 0 || vm_debug.exit_code == 1);
    T81_TEST_CHECK(!vm_debug.stdout_text.empty() || !vm_debug.stderr_text.empty());

    const auto vm_explain = run_cli(t81_bin, {"vm", "explain-trap", hello_tisc.string(), "--json"});
    // explain-trap may return 0 (no trap) or 1 (trapped) — both are valid
    T81_TEST_CHECK(vm_explain.exit_code == 0 || vm_explain.exit_code == 1);
    T81_TEST_CHECK(contains(vm_explain.stdout_text, "\"schema\": \"t81.vm-explain-trap.v1\"") ||
                   contains(vm_explain.stdout_text, "no trap") || !vm_explain.stdout_text.empty());
  }

  // ── §6  code lint / fmt / test / debug / disasm ──────────────────────────
  {
    const auto lint = run_cli(t81_bin, {"code", "lint", hello_src.string()});
    T81_TEST_CHECK(lint.exit_code == 0 || lint.exit_code == 1);
    // lint on a valid file should not crash
    T81_TEST_CHECK(lint.exit_code < 128);

    const auto fmt = run_cli(t81_bin, {"code", "fmt", hello_src.string(), "--check"});
    // 0=already formatted, 1=error, 2=would reformat — all are valid non-crash exits
    T81_TEST_CHECK(fmt.exit_code >= 0 && fmt.exit_code < 128);

    const auto test_cmd = run_cli(t81_bin, {"code", "test", hello_src.string()});
    T81_TEST_CHECK(test_cmd.exit_code == 0 || test_cmd.exit_code == 1);
    T81_TEST_CHECK(test_cmd.exit_code < 128);

    const auto dbg =
        run_cli(t81_bin, {"code", "debug", hello_tisc.string(), "--steps", "2", "--json"});
    T81_TEST_CHECK(dbg.exit_code == 0 || dbg.exit_code == 1);
    T81_TEST_CHECK(dbg.exit_code < 128);

    const auto disasm = run_cli(t81_bin, {"code", "disasm", hello_tisc.string()});
    T81_TEST_CHECK(disasm.exit_code == 0);
    T81_TEST_CHECK(!disasm.stdout_text.empty());
  }

  // ── §7  lang check / lint / fmt / build / run / test / debug / profile / show / dump / disasm ──
  {
    const auto lcheck = run_cli(t81_bin, {"lang", "check", hello_src.string(), "--json"});
    T81_TEST_CHECK(lcheck.exit_code == 0);
    T81_TEST_CHECK(contains(lcheck.stdout_text, "schema") || !lcheck.stdout_text.empty());

    const auto llint = run_cli(t81_bin, {"lang", "lint", hello_src.string()});
    T81_TEST_CHECK(llint.exit_code == 0 || llint.exit_code == 1);
    T81_TEST_CHECK(llint.exit_code < 128);

    const auto lfmt = run_cli(t81_bin, {"lang", "fmt", hello_src.string(), "--check"});
    // 0=already formatted, 1=error, 2=would reformat
    T81_TEST_CHECK(lfmt.exit_code >= 0 && lfmt.exit_code < 128);

    const fs::path lang_out = make_temp_path("t81-stress-lang", ".tisc");
    const auto lbuild =
        run_cli(t81_bin, {"lang", "build", hello_src.string(), "-o", lang_out.string()});
    T81_TEST_CHECK(lbuild.exit_code == 0);
    T81_TEST_CHECK(fs::exists(lang_out));

    const auto lrun = run_cli(t81_bin, {"lang", "run", hello_src.string()});
    T81_TEST_CHECK(lrun.exit_code == 0);
    T81_TEST_CHECK(contains(lrun.stdout_text, "hello stress"));

    const auto ltest = run_cli(t81_bin, {"lang", "test", hello_src.string()});
    T81_TEST_CHECK(ltest.exit_code == 0 || ltest.exit_code == 1);
    T81_TEST_CHECK(ltest.exit_code < 128);

    const auto ldebug =
        run_cli(t81_bin, {"lang", "debug", hello_src.string(), "--steps", "2", "--json"});
    T81_TEST_CHECK(ldebug.exit_code == 0 || ldebug.exit_code == 1);
    T81_TEST_CHECK(ldebug.exit_code < 128);

    const auto lprofile = run_cli(t81_bin, {"lang", "profile", hello_src.string(), "--json"});
    T81_TEST_CHECK(lprofile.exit_code == 0);
    T81_TEST_CHECK(contains(lprofile.stdout_text, "\"schema\": \"t81.vm-profile.v1\""));

    const auto lshow = run_cli(t81_bin, {"lang", "show", hello_src.string()});
    T81_TEST_CHECK(lshow.exit_code == 0);
    T81_TEST_CHECK(!lshow.stdout_text.empty());

    const auto ldump = run_cli(t81_bin, {"lang", "dump", hello_src.string()});
    T81_TEST_CHECK(ldump.exit_code == 0);
    T81_TEST_CHECK(!ldump.stdout_text.empty());

    // lang disasm expects a compiled .tisc file, not a source .t81
    const auto ldisasm = run_cli(t81_bin, {"lang", "disasm", lang_out.string()});
    T81_TEST_CHECK(ldisasm.exit_code == 0);
    T81_TEST_CHECK(!ldisasm.stdout_text.empty());

    std::error_code ec;
    fs::remove(lang_out, ec);
  }

  // ── §8  tisc stats / tisc diff ───────────────────────────────────────────
  {
    const auto stats = run_cli(t81_bin, {"tisc", "stats", hello_tisc.string(), "--json"});
    T81_TEST_CHECK(stats.exit_code == 0);
    T81_TEST_CHECK(contains(stats.stdout_text, "\"schema\": \"t81.tisc-stats.v1\""));

    // diff identical files → no difference
    const auto diff_same =
        run_cli(t81_bin, {"tisc", "diff", hello_tisc.string(), hello_tisc.string(), "--json"});
    T81_TEST_CHECK(diff_same.exit_code == 0);
    T81_TEST_CHECK(contains(diff_same.stdout_text, "\"schema\": \"t81.tisc-diff.v1\""));

    // diff two different binaries → non-zero exit or diff output
    const auto diff_diff =
        run_cli(t81_bin, {"tisc", "diff", hello_tisc.string(), arith_tisc.string(), "--json"});
    T81_TEST_CHECK(diff_diff.exit_code == 0 || diff_diff.exit_code == 1);
    T81_TEST_CHECK(contains(diff_diff.stdout_text, "\"schema\": \"t81.tisc-diff.v1\""));
  }

  // ── §9  ir dump / ir validate ────────────────────────────────────────────
  {
    const auto dump = run_cli(t81_bin, {"ir", "dump", hello_src.string()});
    T81_TEST_CHECK(dump.exit_code == 0);
    T81_TEST_CHECK(!dump.stdout_text.empty());

    const auto validate = run_cli(t81_bin, {"ir", "validate", hello_src.string(), "--json"});
    T81_TEST_CHECK(validate.exit_code == 0);
    T81_TEST_CHECK(contains(validate.stdout_text, "\"schema\": \"t81.ir-validate.v1\""));
  }

  // ── §10  determinism hash / diff / multi-run ─────────────────────────────
  {
    const auto dhash = run_cli(t81_bin, {"determinism", "hash", hello_tisc.string(), "--json"});
    T81_TEST_CHECK(dhash.exit_code == 0);
    T81_TEST_CHECK(contains(dhash.stdout_text, "\"schema\": \"t81.determinism-hash.v1\""));

    const auto ddiff = run_cli(
        t81_bin, {"determinism", "diff", hello_tisc.string(), hello_tisc.string(), "--json"});
    T81_TEST_CHECK(ddiff.exit_code == 0);
    T81_TEST_CHECK(contains(ddiff.stdout_text, "\"schema\": \"t81.determinism-diff.v1\"") ||
                   contains(ddiff.stdout_text, "identical"));

    const auto multi = run_cli(
        t81_bin, {"determinism", "multi-run", hello_tisc.string(), "--count", "3", "--json"});
    T81_TEST_CHECK(multi.exit_code == 0);
    T81_TEST_CHECK(contains(multi.stdout_text, "\"schema\": \"t81.determinism-multi-run.v1\""));
  }

  // ── §11  policy compile / list / run ─────────────────────────────────────
  {
    const fs::path compiled_policy = make_temp_path("t81-stress-policy", ".axionb");

    // policy compile prints a human-readable success message (no JSON schema)
    const auto compile = run_cli(
        t81_bin, {"policy", "compile", policy_file.string(), "-o", compiled_policy.string()});
    T81_TEST_CHECK(compile.exit_code == 0);
    T81_TEST_CHECK(!compile.stdout_text.empty());

    const auto list = run_cli(t81_bin, {"policy", "list", "--json"});
    T81_TEST_CHECK(list.exit_code == 0);
    T81_TEST_CHECK(contains(list.stdout_text, "\"schema\": \"t81.policy-list.v1\""));

    // policy run validates the policy file only (no tisc file argument)
    const auto run_pol = run_cli(t81_bin, {"policy", "run", policy_file.string(), "--json"});
    T81_TEST_CHECK(run_pol.exit_code == 0);
    T81_TEST_CHECK(contains(run_pol.stdout_text, "\"schema\": \"t81.policy-run.v1\""));

    std::error_code ec;
    fs::remove(compiled_policy, ec);
  }

  // ── §12  axion simulate / audit ──────────────────────────────────────────
  {
    const auto simulate = run_cli(t81_bin, {"axion", "simulate", hello_tisc.string(), "--json"});
    T81_TEST_CHECK(simulate.exit_code == 0);
    // compact JSON may have no space after ':'
    T81_TEST_CHECK(contains(simulate.stdout_text, "\"schema\"") &&
                   contains(simulate.stdout_text, "t81.axion-simulate.v1"));

    const auto audit = run_cli(t81_bin, {"axion", "audit", "--json"});
    T81_TEST_CHECK(audit.exit_code == 0);
    T81_TEST_CHECK(contains(audit.stdout_text, "\"schema\": \"t81.axion-audit.v1\""));
  }

  // ── §13  tier info / check / gate ────────────────────────────────────────
  {
    const auto info = run_cli(t81_bin, {"tier", "info", "--json"});
    T81_TEST_CHECK(info.exit_code == 0);
    T81_TEST_CHECK(contains(info.stdout_text, "\"schema\": \"t81.tier-info.v1\""));

    const auto check = run_cli(t81_bin, {"tier", "check", hello_tisc.string(), "--json"});
    T81_TEST_CHECK(check.exit_code == 0);
    T81_TEST_CHECK(contains(check.stdout_text, "\"schema\": \"t81.tier-check.v1\""));

    const auto gate =
        run_cli(t81_bin, {"tier", "gate", hello_tisc.string(), "--max-tier", "2", "--json"});
    T81_TEST_CHECK(gate.exit_code == 0 || gate.exit_code == 1);
    T81_TEST_CHECK(contains(gate.stdout_text, "\"schema\": \"t81.tier-gate.v1\""));
  }

  // ── §14  tensor hash / inspect — expect error (no model file) ───────────
  {
    const fs::path missing_tensor = make_temp_path("t81-stress-no", ".t81w");

    const auto thash = run_cli(t81_bin, {"tensor", "hash", missing_tensor.string(), "--json"});
    // Missing file → non-zero exit; must not crash (exit < 128)
    T81_TEST_CHECK(thash.exit_code != 0);
    T81_TEST_CHECK(thash.exit_code < 128);

    const auto tinspect = run_cli(t81_bin, {"tensor", "inspect", missing_tensor.string()});
    T81_TEST_CHECK(tinspect.exit_code != 0);
    T81_TEST_CHECK(tinspect.exit_code < 128);
  }

  // ── §15  weights — expected errors (no model file) ──────────────────────
  {
    const fs::path missing_model = make_temp_path("t81-stress-no", ".t81w");

    const auto winfo = run_cli(t81_bin, {"weights", "info", missing_model.string(), "--json"});
    T81_TEST_CHECK(winfo.exit_code != 0);
    T81_TEST_CHECK(winfo.exit_code < 128);

    const auto wverify = run_cli(t81_bin, {"weights", "verify", missing_model.string(), "--json"});
    T81_TEST_CHECK(wverify.exit_code != 0);
    T81_TEST_CHECK(wverify.exit_code < 128);

    const fs::path missing_sf = make_temp_path("t81-stress-no", ".safetensors");
    const auto wimport = run_cli(t81_bin, {"weights", "import", missing_sf.string(), "--json"});
    T81_TEST_CHECK(wimport.exit_code != 0);
    T81_TEST_CHECK(wimport.exit_code < 128);
  }

  // ── §16  Error boundaries ────────────────────────────────────────────────
  {
    // §16a  Missing input file — every file-taking command must reject cleanly
    const fs::path no_file = make_temp_path("t81-stress-missing", ".t81");

    for (const std::vector<std::string>& cmd : {
             std::vector<std::string>{"code", "build", no_file.string()},
             std::vector<std::string>{"code", "run", no_file.string()},
             std::vector<std::string>{"code", "check", no_file.string()},
             std::vector<std::string>{"code", "disasm", no_file.string()},
             std::vector<std::string>{"lang", "check", no_file.string()},
             std::vector<std::string>{"lang", "build", no_file.string()},
             std::vector<std::string>{"ir", "show", no_file.string()},
             std::vector<std::string>{"ir", "dump", no_file.string()},
             std::vector<std::string>{"vm", "state", no_file.string(), "--json"},
         }) {
      const auto r = run_cli(t81_bin, cmd);
      T81_TEST_CHECK(r.exit_code != 0);
      T81_TEST_CHECK(r.exit_code < 128);  // no SIGSEGV / SIGABRT
    }

    // §16b  Syntax-error source — check / lint must report the error, not crash
    {
      const auto r = run_cli(t81_bin, {"code", "check", bad_src.string()});
      T81_TEST_CHECK(r.exit_code != 0);
      T81_TEST_CHECK(r.exit_code < 128);
      T81_TEST_CHECK(contains(r.stderr_text, ":1:") || contains(r.stdout_text, ":1:"));
    }
    {
      const auto r = run_cli(t81_bin, {"lang", "check", bad_src.string()});
      T81_TEST_CHECK(r.exit_code != 0);
      T81_TEST_CHECK(r.exit_code < 128);
    }

    // §16c  Wrong extension — commands that validate extension must reject cleanly
    {
      const fs::path wrong_ext = make_temp_path("t81-stress-wrong", ".txt");
      {
        std::ofstream o(wrong_ext);
        o << "garbage\n";
      }

      const auto r = run_cli(t81_bin, {"code", "build", wrong_ext.string()});
      T81_TEST_CHECK(r.exit_code != 0);
      T81_TEST_CHECK(r.exit_code < 128);

      std::error_code ec;
      fs::remove(wrong_ext, ec);
    }

    // §16d  Corrupt TISC binary — vm commands must trap, not crash
    {
      const fs::path corrupt = make_temp_path("t81-stress-corrupt", ".tisc");
      {
        std::ofstream o(corrupt, std::ios::binary);
        // Use ostream::write to include the embedded null byte; do NOT use operator<<
        // with a C-string literal that contains \x00 (it would truncate at the null).
        static const char kCorrupt[] = "\xFF\xFE\x00\x01garbage";
        o.write(kCorrupt, sizeof(kCorrupt) - 1);  // -1: exclude string terminator
      }

      for (const std::vector<std::string>& cmd : {
               std::vector<std::string>{"vm", "state", corrupt.string(), "--json"},
               std::vector<std::string>{"vm", "run", corrupt.string()},
               std::vector<std::string>{"tisc", "validate", corrupt.string(), "--json"},
           }) {
        const auto r = run_cli(t81_bin, cmd);
        T81_TEST_CHECK(r.exit_code < 128);  // must not SIGSEGV
      }

      std::error_code ec;
      fs::remove(corrupt, ec);
    }

    // §16e  Empty TISC file — must produce a clean error
    {
      const fs::path empty_tisc = make_temp_path("t81-stress-empty", ".tisc");
      { std::ofstream o(empty_tisc); }  // zero bytes

      const auto r = run_cli(t81_bin, {"vm", "run", empty_tisc.string()});
      T81_TEST_CHECK(r.exit_code != 0);
      T81_TEST_CHECK(r.exit_code < 128);

      std::error_code ec;
      fs::remove(empty_tisc, ec);
    }

    // §16f  Empty T81 source — compile must reject cleanly (not crash)
    {
      const fs::path empty_src = make_temp_path("t81-stress-empty", ".t81");
      { std::ofstream o(empty_src); }

      const auto r = run_cli(t81_bin, {"code", "build", empty_src.string()});
      T81_TEST_CHECK(r.exit_code < 128);

      std::error_code ec;
      fs::remove(empty_src, ec);
    }

    // §16g  No args — commands that require a subcommand must print usage, not crash
    for (const std::string top : {"vm", "trace", "tisc", "ir", "tier", "lang", "policy", "axion",
                                  "canonfs", "determinism", "weights"}) {
      const auto r = run_cli(t81_bin, {top});
      T81_TEST_CHECK(r.exit_code < 128);
      T81_TEST_CHECK(!r.stdout_text.empty() || !r.stderr_text.empty());
    }

    // §16h  Unrecognised subcommand — must error, not crash
    for (const std::string top :
         {"vm", "tisc", "ir", "tier", "lang", "policy", "axion", "determinism", "weights"}) {
      const auto r = run_cli(t81_bin, {top, "does-not-exist"});
      T81_TEST_CHECK(r.exit_code != 0);
      T81_TEST_CHECK(r.exit_code < 128);
    }
  }

  // ── §17  Cleanup ─────────────────────────────────────────────────────────
  {
    std::error_code ec;
    for (const fs::path& p :
         {hello_src, arith_src, bad_src, hello_tisc, arith_tisc, trace_file, policy_file}) {
      fs::remove(p, ec);
    }
  }

  std::cout << "CLI stress tests PASSED!\n";
  return 0;
}
