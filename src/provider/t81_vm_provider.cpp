#include "provider/t81_vm_provider.hpp"

#include <chrono>
#include <memory>
#include <sstream>

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/config.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/vm/vm.hpp"
#include "vm/internal/tensor_helpers.hpp"

namespace t81::provider {

T81VmProvider::T81VmProvider(t81::axion::Policy vm_policy) : vm_policy_(std::move(vm_policy)) {
  descriptor_.provider_id = "t81.vm.tisc";
  descriptor_.provider_version = T81_VERSION_STR;
  descriptor_.runtime_environment = std::string("t81vm-tisc-v1.9.0-") + T81_VERSION_STR;
  descriptor_.execution_class = ExecutionClass::Deterministic;
  descriptor_.isolation_mode = IsolationMode::InProcessReadOnly;
  descriptor_.capabilities.supported_operations = {"subgraph.run"};
  descriptor_.capabilities.supported_input_types = {"application/x-t81-tisc"};
  descriptor_.capabilities.supported_output_types = {"application/json"};
  descriptor_.capabilities.supports_cancellation = true;
  descriptor_.capabilities.supports_streaming = false;
  descriptor_.capabilities.requires_network = false;
  descriptor_.capabilities.requires_user_consent = false;
  descriptor_.capabilities.max_input_bytes = 1024 * 1024;
  descriptor_.capabilities.max_output_tokens = 0;
}

ProviderResult T81VmProvider::execute(const ProviderRequest& request,
                                      const ExecutionContext& context,
                                      const CancellationToken& cancel) {
  invocations_.fetch_add(1, std::memory_order_acq_rel);

  ProviderResult res;
  auto decoded = t81::tisc::decode(request.input.bytes);
  if (!decoded) {
    res.ok = false;
    res.trit = -1;
    res.failure_code = FailureCode::ProviderFault;
    res.failure_detail = "failed to decode TISC bytecode payload";
    return res;
  }

  auto inner = std::make_unique<t81::axion::PolicyEngine>(vm_policy_);
  auto hook = t81::axion::make_ai_hook_engine(std::move(inner));
  auto vm = t81::vm::make_interpreter_vm(std::move(hook));
  vm->load_program(*decoded);

  const std::uint64_t step_limit =
      context.budget.max_steps > 0 ? context.budget.max_steps : 100000ULL;
  const auto started = std::chrono::steady_clock::now();
  const auto before_telem = t81::vm::internal::dualrail_telemetry();

  std::uint64_t steps = 0;
  while (!vm->state().halted) {
    if (cancel.is_cancelled()) {
      res.ok = false;
      res.trit = -1;
      res.failure_code = FailureCode::Cancelled;
      res.failure_detail = "cancelled during VM execution";
      res.steps_consumed = steps;
      return res;
    }

    if (context.budget.timeout_ms > 0) {
      const auto elapsed_ms =
          static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - started)
                                         .count());
      if (elapsed_ms > context.budget.timeout_ms) {
        res.ok = false;
        res.trit = -1;
        res.failure_code = FailureCode::TimedOut;
        res.failure_detail = "VM execution exceeded timeout_ms";
        res.steps_consumed = steps;
        return res;
      }
    }

    if (steps >= step_limit) {
      res.ok = false;
      res.trit = -1;
      res.failure_code = FailureCode::BudgetExhausted;
      res.failure_detail = "VM execution exceeded max_steps budget";
      res.steps_consumed = steps;
      return res;
    }

    auto step_res = vm->step();
    ++steps;
    if (!step_res) {
      res.ok = false;
      res.trit = -1;
      res.failure_code = FailureCode::ProviderFault;
      res.failure_detail =
          "VM trapped with code " + std::to_string(static_cast<int>(step_res.error()));
      res.steps_consumed = steps;
      return res;
    }
  }

  const auto after_telem = t81::vm::internal::dualrail_telemetry();
  const std::uint64_t fallbacks = after_telem.fallback_count >= before_telem.fallback_count
                                      ? (after_telem.fallback_count - before_telem.fallback_count)
                                      : 0;

  const std::int64_t r0 =
      (!vm->state().contexts.empty() && vm->state().current_context < vm->state().contexts.size())
          ? vm->state().contexts[vm->state().current_context].registers[0]
          : 0;
  const std::int64_t r1 =
      (!vm->state().contexts.empty() && vm->state().current_context < vm->state().contexts.size())
          ? vm->state().contexts[vm->state().current_context].registers[1]
          : 0;

  std::ostringstream out_json;
  out_json << "{"
           << "\"halted\":true,"
           << "\"r0\":" << r0 << ","
           << "\"r1\":" << r1 << ","
           << "\"steps\":" << steps << "}";
  const std::string out_str = out_json.str();

  res.ok = true;
  res.trit = 1;
  res.failure_code = FailureCode::None;
  res.steps_consumed = steps;
  res.dualrail_fallback_count = fallbacks;
  res.output.media_type = "application/json";
  res.output.bytes.resize(out_str.size());
  for (std::size_t i = 0; i < out_str.size(); ++i) {
    res.output.bytes[i] = static_cast<std::byte>(out_str[i]);
  }
  return res;
}

}  // namespace t81::provider
