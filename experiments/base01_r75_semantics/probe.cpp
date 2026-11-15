// Correctness observations only. No timing, workload rewriting or source overlay.
#define main audited_determinism_test_main
#include "vm_workload_determinism_tiers_test.cpp"
#undef main
#include <functional>
#include <iomanip>
#include <stdexcept>
#include "t81/axion/reasons.hpp"
#include "t81/jit/jit.hpp"

using t81::tisc::Opcode;
using t81::vm::IVirtualMachine;

static void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
static void quote(std::string_view value) {
  std::cout << '"';
  for (unsigned char c : value) {
    if (c == '"' || c == '\\')
      std::cout << '\\' << char(c);
    else if (c == '\n')
      std::cout << "\\n";
    else if (c == '\r')
      std::cout << "\\r";
    else if (c == '\t')
      std::cout << "\\t";
    else if (c < 32)
      throw std::runtime_error("unexpected control byte");
    else
      std::cout << char(c);
  }
  std::cout << '"';
}
static void snapshot(const t81::vm::State& s) {
  const auto& c = s.contexts.at(0);
  std::cout << "{\"pc\":" << c.pc << ",\"r75\":" << c.registers[75]
            << ",\"halted\":" << (s.halted ? "true" : "false")
            << ",\"context_halted\":" << (c.halted ? "true" : "false") << ",\"registers\":[";
  for (size_t i = 0; i < 48; ++i) {
    if (i) std::cout << ',';
    std::cout << c.registers[i];
  }
  std::cout << "],\"tags\":[";
  for (size_t i = 0; i < 48; ++i) {
    if (i) std::cout << ',';
    std::cout << int(c.register_tags[i]);
  }
  std::cout << "],\"floats\":[";
  for (size_t i = 0; i < s.floats.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << s.floats[i];
  }
  std::cout << "],\"printed\":[";
  for (size_t i = 0; i < s.printed_output.size(); ++i) {
    if (i) std::cout << ',';
    quote(s.printed_output[i]);
  }
  std::cout << "]}";
}
struct Observation {
  std::string reason;
  size_t pc;
  int opcode;
  size_t count;
  int64_t r75;
};
class ObservingEngine final : public t81::axion::Engine {
public:
  IVirtualMachine* machine = nullptr;
  bool deny_add = false;
  bool deny_trace = false;
  std::vector<Observation> events;
  std::unique_ptr<t81::axion::Engine> delegate = t81::axion::make_allow_all_engine();
  t81::axion::Verdict evaluate(const t81::axion::SyscallContext& ctx) override {
    events.push_back({ctx.syscall, ctx.pc, int(ctx.next_opcode), ctx.instruction_count,
                      machine ? machine->state().contexts.at(0).registers[75] : -1});
    if (deny_trace && ctx.syscall == t81::axion::reasons::kStep && !events.empty() &&
        events.front().reason == t81::axion::reasons::kJitTraceEnter)
      return {t81::axion::VerdictKind::Deny, "synthetic trace denial"};
    if (deny_add && ctx.syscall == t81::axion::reasons::kStep && ctx.pc == 2)
      return {t81::axion::VerdictKind::Deny, "synthetic correctness denial"};
    return delegate->evaluate(ctx);
  }
};
static void events_json(const std::vector<Observation>& events) {
  std::cout << '[';
  for (size_t i = 0; i < events.size(); ++i) {
    if (i) std::cout << ',';
    const auto& e = events[i];
    std::cout << '[';
    quote(e.reason);
    std::cout << ',' << e.pc << ',' << e.opcode << ',' << e.count << ',' << e.r75 << ']';
  }
  std::cout << ']';
}
static void run_case(const std::string& name, const t81::tisc::Program& p, bool expected_halt,
                     bool deny = false, bool deny_trace = false) {
  // load_program replaces/destroys an injected engine for policy-bearing programs.
  // Never retain a pointer to that engine; keep those programs wholly unobserved.
  ObservingEngine* observer = nullptr;
  std::unique_ptr<IVirtualMachine> machine;
  if (p.axion_policy_text.empty()) {
    auto engine = std::make_unique<ObservingEngine>();
    observer = engine.get();
    observer->deny_add = deny;
    observer->deny_trace = deny_trace;
    machine = t81::vm::make_interpreter_vm(std::move(engine));
    observer->machine = machine.get();
  } else {
    require(!deny && !deny_trace, "denial probe requires no embedded policy");
    machine = t81::vm::make_interpreter_vm();
  }
  machine->load_program(p);
  require(machine->state().contexts.size() == 1, "expected one context");
  std::cout << "{\"type\":\"program\",\"name\":";
  quote(name);
  std::cout << ",\"observer_active\":" << (observer ? "true" : "false") << ",\"instructions\":[";
  for (size_t i = 0; i < p.insns.size(); ++i) {
    if (i) std::cout << ',';
    const auto& x = p.insns[i];
    std::cout << '[' << int(x.opcode) << ',' << x.a << ',' << x.b << ',' << x.c << ','
              << int(x.literal_kind) << ']';
  }
  std::cout << "],\"policy\":";
  quote(p.axion_policy_text);
  std::cout << ",\"initial\":";
  snapshot(machine->state());
  std::cout << "}\n";
  size_t steps = 0;
  bool fault = false;
  while (!machine->state().halted && steps < 5000) {
    const auto old_trace = machine->state().trace.size();
    if (observer) observer->events.clear();
    std::cout << "{\"type\":\"step\",\"name\":";
    quote(name);
    std::cout << ",\"index\":" << steps << ",\"before\":";
    snapshot(machine->state());
    auto result = machine->step();
    ++steps;
    std::cout << ",\"success\":" << (result.has_value() ? "true" : "false") << ",\"after\":";
    snapshot(machine->state());
    std::cout << ",\"events\":";
    events_json(observer ? observer->events : std::vector<Observation>{});
    std::cout << ",\"new_trace\":[";
    const auto& s = machine->state();
    for (size_t i = old_trace; i < s.trace.size(); ++i) {
      if (i > old_trace) std::cout << ',';
      std::cout << '[' << s.trace[i].pc << ',' << int(s.trace[i].opcode) << ','
                << (s.trace[i].trap.has_value() ? int(*s.trace[i].trap) : -1) << ']';
    }
    std::cout << "]}\n";
    if (!result.has_value()) {
      fault = true;
      break;
    }
  }
  require(expected_halt ? machine->state().halted : !machine->state().halted,
          "unexpected completion state");
  require(expected_halt ? !fault : fault, "unexpected fault state");
  // Compare observation engine with the default engine on unchanged programs.
  auto plain = t81::vm::make_interpreter_vm();
  plain->load_program(p);
  auto completed = plain->run_to_halt(5000);
  if (expected_halt) {
    require(completed.has_value() && plain->state().halted, "unobserved run incomplete");
    require(plain->state().contexts[0].registers == machine->state().contexts[0].registers,
            "observer changed final registers");
    require(plain->state().printed_output == machine->state().printed_output,
            "observer changed output");
    require(plain->state().contexts[0].register_tags == machine->state().contexts[0].register_tags,
            "observer changed tags");
    require(plain->state().contexts[0].pc == machine->state().contexts[0].pc,
            "observer changed PC");
    require(plain->state().floats == machine->state().floats, "observer changed floats");
    require(plain->state().memory == machine->state().memory, "observer changed memory");
    require(plain->state().trace.size() == machine->state().trace.size(),
            "observer changed trace size");
    require(plain->state().axion_log.size() == machine->state().axion_log.size(),
            "observer changed log size");
    for (size_t i = 0; i < plain->state().trace.size(); ++i) {
      const auto& a = plain->state().trace[i];
      const auto& b = machine->state().trace[i];
      require(a.pc == b.pc && a.opcode == b.opcode && a.trap == b.trap,
              "observer changed trace content");
    }
    for (size_t i = 0; i < plain->state().axion_log.size(); ++i) {
      const auto& a = plain->state().axion_log[i];
      const auto& b = machine->state().axion_log[i];
      require(a.opcode == b.opcode && a.tag == b.tag && a.value == b.value &&
                  a.verdict.kind == b.verdict.kind && a.verdict.reason == b.verdict.reason,
              "observer changed log content");
    }
  }
  std::cout << "{\"type\":\"final\",\"name\":";
  quote(name);
  std::cout << ",\"step_calls\":" << steps << ",\"trace_entries\":" << machine->state().trace.size()
            << ",\"state\":";
  snapshot(machine->state());
  std::cout << "}\n";
}
static void completion_and_reload() {
  auto machine = t81::vm::make_interpreter_vm();
  machine->load_program(micro_program());
  auto zero = machine->run_to_halt(0);
  require(zero.has_value() && !machine->state().halted, "zero budget counterexample missing");
  auto partial = machine->run_to_halt(4);
  require(partial.has_value() && !machine->state().halted,
          "budget exhaustion counterexample missing");
  auto done = machine->run_to_halt(1);
  require(done.has_value() && machine->state().halted && machine->state().contexts[0].pc == 5,
          "Halt check failed");
  machine->load_program(micro_program());
  auto initial = machine->state().contexts[0].registers[75];
  require(initial == 5, "reload stale R75 counterexample missing");
  require(machine->step().has_value() && machine->state().contexts[0].registers[75] == 1,
          "reload counter not reset");
  std::cout << "{\"type\":\"counterexample\",\"name\":\"budget-and-reload\",\"zero_budget_success_"
               "not_halted\":true,\"four_step_success_not_halted\":true,\"reloaded_initial_r75\":"
            << initial << ",\"after_first_step_r75\":1}\n";
}
static void side_exit_and_reused_trace() {
  std::vector<Opcode> boundaries{Opcode::AxRead, Opcode::AxSet, Opcode::AxVerify, Opcode::AxReport,
                                 Opcode::AgentInvoke};
  for (auto opcode : boundaries) {
    t81::vm::JitCompiler compiler;
    compiler.start_tracing(0);
    compiler.record_instruction({opcode, 40, 0, 0});
    require(!compiler.is_tracing() && !compiler.compile(), "boundary unexpectedly recordable");
    compiler.start_tracing(0);
    compiler.record_instruction({Opcode::Add, 40, 41, 42});
    compiler.record_instruction({opcode, 40, 0, 0});
    auto trace = compiler.compile();
    require(trace && trace->size() == 1 && trace->instructions()[0].opcode == Opcode::Add,
            "Axion boundary appended to trace");
    t81::vm::State state;
    state.contexts.emplace_back();
    auto denied = trace->execute(state, [](auto, const auto&, auto) { return false; });
    require(denied.exit_kind == t81::vm::JitTrace::ExitKind::PolicyDeny &&
                denied.instructions_executed == 0,
            "trace denial incremented");
  }
  auto observer = std::make_unique<ObservingEngine>();
  auto* engine = observer.get();
  auto machine = t81::vm::make_interpreter_vm(std::move(observer));
  engine->machine = machine.get();
  machine->load_program(meso_program());
  require(machine->run_to_halt(5000).has_value() && machine->state().halted, "meso incomplete");
  machine->load_program(micro_program());
  engine->events.clear();
  require(machine->run_to_halt(5000).has_value() && machine->state().halted,
          "reuse run incomplete");
  const auto& state = machine->state();
  require(state.contexts[0].pc == 6 && state.contexts[0].registers[75] == 3 &&
              state.contexts[0].registers[43] == 0,
          "reused trace counterexample missing");
  std::cout << "{\"type\":\"counterexample\",\"name\":\"reused-trace\",\"state\":";
  snapshot(state);
  std::cout << ",\"events\":";
  events_json(engine->events);
  std::cout << "}\n";
  auto empty = t81::vm::make_interpreter_vm();
  t81::tisc::Program no_instructions;
  empty->load_program(no_instructions);
  require(empty->run_to_halt(1).has_value() && empty->state().halted &&
              empty->state().contexts[0].registers[75] == 0 && empty->state().trace.empty(),
          "implicit halt count changed");
  std::cout
      << "{\"type\":\"counterexample\",\"name\":\"side-exit-and-implicit-halt\",\"axion_boundaries_"
         "not_recorded\":5,\"trace_denial_count\":0,\"empty_program_halted_r75\":0}\n";
}

int main(int argc, char** argv) {
  if (argc != 2 || std::string(argv[1]) != "--validate-only") {
    std::cerr << "correctness only: --validate-only required\n";
    return 2;
  }
  if (std::getenv("T81_CANONFS_ROOT")) {
    std::cerr << "unset T81_CANONFS_ROOT\n";
    return 2;
  }
  try {
    run_case("workload", workload_program(), true);
    run_case("micro", micro_program(), true);
    run_case("meso", meso_program(), true);
    run_case("mixed", mixed_program(), true);
    run_case("policy-heavy", policy_heavy_program(), true);
    run_case("tensor-access", tensor_access_program(), true);
    // Separate negative correctness probes, not substituted historical workloads.
    t81::tisc::Program guard;
    guard.insns = {{Opcode::LoadImm, 40, 0, 0},  {Opcode::LoadImm, 41, 1, 0},
                   {Opcode::LoadImm, 42, 55, 0}, {Opcode::Sub, 44, 42, 40},
                   {Opcode::Div, 46, 42, 44},    {Opcode::Add, 40, 40, 41},
                   {Opcode::Jump, 3, 0, 0},      {Opcode::Halt, 0, 0, 0}};
    run_case("synthetic-guard-retry", guard, false);
    run_case("synthetic-policy-denial", micro_program(), false, true);
    run_case("synthetic-trace-policy-denial", meso_program(), false, false, true);
    completion_and_reload();
    side_exit_and_reused_trace();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
