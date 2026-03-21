#include "internal/gc_helpers.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace t81::vm::internal {

GcReclaimCounts mark_and_sweep(State& state) {
  std::vector<bool> marked_tensors(state.tensors.size(), false);
  std::vector<bool> marked_infinite_forms(state.infinite_forms.size(), false);
  std::vector<bool> visited_options(state.options.size(), false);
  std::vector<bool> visited_results(state.results.size(), false);
  std::vector<bool> visited_enums(state.enums.size(), false);

  auto mark_tensor = [&](std::int64_t handle) {
    if (handle <= 0) return;
    std::size_t idx = static_cast<std::size_t>(handle - 1);
    if (idx < marked_tensors.size()) {
      marked_tensors[idx] = true;
    }
  };

  auto mark_infinite = [&](std::int64_t handle) {
    if (handle <= 0) return;
    std::size_t idx = static_cast<std::size_t>(handle - 1);
    if (idx < marked_infinite_forms.size()) {
      marked_infinite_forms[idx] = true;
    }
  };

  std::function<void(ValueTag, std::int64_t)> scan_value = [&](ValueTag tag, std::int64_t val) {
    switch (tag) {
      case ValueTag::TensorHandle:
        mark_tensor(val);
        break;
      case ValueTag::OptionHandle: {
        if (val <= 0) return;
        std::size_t idx = static_cast<std::size_t>(val - 1);
        if (idx < state.options.size() && !visited_options[idx]) {
          visited_options[idx] = true;
          scan_value(state.options[idx].payload_tag, state.options[idx].payload);
        }
        break;
      }
      case ValueTag::ResultHandle: {
        if (val <= 0) return;
        std::size_t idx = static_cast<std::size_t>(val - 1);
        if (idx < state.results.size() && !visited_results[idx]) {
          visited_results[idx] = true;
          scan_value(state.results[idx].payload_tag, state.results[idx].payload);
        }
        break;
      }
      case ValueTag::EnumHandle: {
        if (val <= 0) return;
        std::size_t idx = static_cast<std::size_t>(val - 1);
        if (idx < state.enums.size() && !visited_enums[idx]) {
          visited_enums[idx] = true;
          if (state.enums[idx].has_payload) {
            scan_value(state.enums[idx].payload_tag, state.enums[idx].payload);
          }
        }
        break;
      }
      case ValueTag::InfiniteHandle:
        mark_infinite(val);
        break;
      default:
        break;
    }
  };

  for (const auto& ctx : state.contexts) {
    for (std::size_t i = 0; i < ctx.registers.size(); ++i) {
      scan_value(ctx.register_tags[i], ctx.registers[i]);
    }
  }

  for (std::size_t i = 0; i < state.memory.size(); ++i) {
    scan_value(state.memory_tags[i], state.memory[i]);
  }

  for (const auto& snap : state.reflection_snapshots) {
    for (std::size_t i = 0; i < snap.registers.size(); ++i) {
      scan_value(snap.register_tags[i], snap.registers[i]);
    }
  }

  GcReclaimCounts counts{};
  for (std::size_t i = 0; i < state.tensors.size(); ++i) {
    if (state.tensors[i].has_value() && !marked_tensors[i]) {
      state.total_tensor_elements -= state.tensors[i]->data().size();
      state.tensors[i] = std::nullopt;
      state.free_tensor_indices.push_back(i);
      ++counts.tensors;
    }
  }

  for (std::size_t i = 0; i < state.infinite_forms.size(); ++i) {
    if (state.infinite_forms[i].has_value() && !marked_infinite_forms[i]) {
      state.metrics.total_infinite_forms--;
      state.infinite_forms[i] = std::nullopt;
      state.free_infinite_indices.push_back(i);
      ++counts.infinite_forms;
    }
  }

  return counts;
}

void compact_heap(State& state, std::size_t new_ptr) {
  for (auto& frame : state.heap_frames) {
    frame.first = static_cast<std::int64_t>(new_ptr);
  }
  state.heap_frames.clear();
  state.heap_ptr = new_ptr;
}

}  // namespace t81::vm::internal
