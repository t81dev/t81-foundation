// Isolated adapter: real interpreter, Axion final authorization and persistent CanonFS.
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "t81/canonfs/axion_hook.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/vm/vm.hpp"
using Clock = std::chrono::steady_clock;
using namespace t81;
long long ns(Clock::time_point t) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t).count();
}
std::string read(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) throw std::runtime_error("input unavailable");
  return {std::istreambuf_iterator<char>(f), {}};
}
std::span<const std::byte> bytes(const std::string& s) {
  return std::as_bytes(std::span(s.data(), s.size()));
}
int main(int argc, char** argv) {
  try {
    if (argc != 4) throw std::runtime_error("usage: arm_a CASE|archive INPUT FRESH_STORE");
    const std::string scenario = argv[1], input = read(argv[2]);
    const std::filesystem::path root = argv[3];
    if (std::filesystem::exists(root)) throw std::runtime_error("store must be fresh");
    if (scenario == "archive") {
      const auto begin = Clock::now();
      auto driver = canonfs::make_persistent_driver(root);
      driver->set_axion_hook(canonfs::make_axion_policy_hook("(policy (tier 1))"));
      auto result = driver->write_object(canonfs::ObjectType::RawBlock, bytes(input));
      if (!result) throw std::runtime_error("archive failed");
      std::cout << "{\"storage_ns\":" << ns(begin) << "}\n";
      return 0;
    }
    const std::vector<std::string> cases = {"approved",           "denied",
                                            "malformed",          "malformed-policy",
                                            "replayed",           "interrupted-before",
                                            "interrupted-after",  "revoke-after-compute",
                                            "revoke-after-check", "revoke-after-commit",
                                            "snapshot-control"};
    if (std::find(cases.begin(), cases.end(), scenario) == cases.end())
      throw std::runtime_error("unknown case");
    auto start = Clock::now();
    int units = 0, price = 0;
    std::istringstream fields(input);
    bool valid = bool(fields >> units >> price) && units >= 1 && units <= 1000 && price >= 1 &&
                 price <= 1000 &&
                 input == std::to_string(units) + " " + std::to_string(price) + "\n";
    int amount = 0, limit = scenario == "denied" ? 10000 : 50000;
    bool consumed = false;
    std::string status = "malformed";
    std::vector<std::string> events;
    std::string native_trace;
    long long execution = 0, authorization = 0, storage = 0;
    long long setup = 0;
    auto driver = canonfs::make_persistent_driver(root);
    auto revoke = [&] {
      limit = 10000;
      events.push_back("policy.reduce.10000");
    };
    auto attempt = [&] {
      bool allowed = false;
      driver->set_axion_hook([&](auto kind, const auto& ref) {
        const auto begin = Clock::now();
        // The host owns amount binding, policy freshness and one-shot transaction ID.
        // Axion implements final Write admission; it has no purchase-order DSL.
        const bool host_allow =
            valid && !consumed && amount == units * price && amount > 0 && amount <= limit;
        const std::string text = scenario == "malformed-policy" ? "(broken"
                                 : host_allow ? "(policy (tier 1))"
                                              : "(policy (tier 1) (max-meta-writes 0))";
        auto hook = canonfs::make_axion_policy_hook(text);
        allowed = hook(kind, ref).allow;
        events.push_back(allowed ? "authorize.allow" : "authorize.deny");
        authorization += ns(begin);
        return canonfs::AxionVerdict{allowed, "experiment final authorization"};
      });
      const auto auth_before = authorization;
      const auto begin = Clock::now();
      const std::string payload = "8491 " + std::to_string(amount) + "\n";
      auto result = driver->write_object(canonfs::ObjectType::RawBlock, bytes(payload));
      storage += ns(begin) - (authorization - auth_before);
      if (!result && result.error() != canonfs::Error::CapabilityError)
        throw std::runtime_error("unexpected storage error");
      if (result) {
        consumed = true;
        events.push_back("write.commit");
      }
      return result.has_value();
    };
    if (valid) {
      tisc::Program p;
      p.axion_policy_text = "(policy (tier 1) (max-instructions 16))";
      p.insns = {{tisc::Opcode::LoadImm, 40, units, 0},
                 {tisc::Opcode::LoadImm, 41, price, 0},
                 {tisc::Opcode::Mul, 42, 40, 41},
                 {tisc::Opcode::Halt, 0, 0, 0}};
      auto machine = vm::make_interpreter_vm();
      machine->load_program(p);
      setup = ns(start);
      start = Clock::now();
      if (!machine->run_to_halt(16)) throw std::runtime_error("VM fault");
      execution = ns(start);
      amount = static_cast<int>(machine->state().contexts[0].registers[42]);
      for (const auto& e : machine->state().trace)
        native_trace +=
            std::to_string(e.pc) + " " + std::string(tisc::opcode_name(e.opcode)) + "\\n";
      events.push_back("compute");
      if (scenario == "revoke-after-compute") revoke();
      if (scenario == "revoke-after-check" || scenario == "snapshot-control") {
        events.push_back(amount <= limit ? "precheck.allow" : "precheck.deny");
        revoke();
      }
      if (scenario == "interrupted-before") {
        events.push_back("interrupt.before");
        status = "interrupted";
      } else {
        // Negative control intentionally restores stale authority at the boundary.
        if (scenario == "snapshot-control") limit = 50000;
        status = attempt() ? "committed" : "denied";
        if (scenario == "snapshot-control") limit = 10000;
        if (scenario == "revoke-after-commit") revoke();
        if (scenario == "replayed") (void)attempt();
        if (scenario == "interrupted-after") {
          events.push_back("interrupt.after");
          status = "interrupted";
        }
      }
    } else {
      events.push_back("input.reject");
      setup = ns(start);
    }
    std::cout << "{\"status\":\"" << status << "\",\"amount\":" << amount << ",\"limit\":" << limit
              << ",\"events\":[";
    for (std::size_t i = 0; i < events.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << '"' << events[i] << '"';
    }
    std::cout << "],\"native_trace\":\"" << native_trace << "\",\"measurements\":{"
              << "\"setup_ns\":" << setup << ",\"execution_ns\":" << execution
              << ",\"authorization_ns\":" << authorization << ",\"storage_ns\":" << storage
              << "}}\n";
    if (scenario == "interrupted-before" || scenario == "interrupted-after") {
      std::cout.flush();
      std::_Exit(75);  // Abrupt process death before protocol evidence is archived.
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
