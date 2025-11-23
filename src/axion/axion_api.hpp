#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace hvm {

/// Minimal frame representation inferred from axion_api.cweb
struct Frame {
    std::uint64_t return_addr = 0;
    // Add other fields you actually use in HanoiVM frames.
};

/// Minimal HVMContext inferred from axion_api.cweb.
/// Adapt to match your real context struct.
struct HVMContext {
    std::size_t stack_ptr = 0;   // index of current top frame
    Frame* stack = nullptr;      // pointer to frame array
    std::uint64_t pc = 0;        // current program counter
};

} // namespace hvm

namespace axion {

/// Generate a human-readable annotation for the current frame.
std::string frame_optimize(const hvm::HVMContext& ctx);

/// Predict an optimization score (0.0–1.0) based on recursion depth.
double predict_score(const hvm::HVMContext& ctx);

/// Suggest whether tail recursion should be collapsed.
bool suggest_tail_collapse(const hvm::HVMContext& ctx);

} // namespace axion

// Optional C-compatible wrappers mirroring the original C API

extern "C" {

/// C-style API equivalent to the original axion_frame_optimize.
/// out_annotation must have room for max_len bytes (including null terminator).
void axion_frame_optimize(hvm::HVMContext* ctx,
                          char* out_annotation,
                          std::size_t max_len);

double axion_predict_score(hvm::HVMContext* ctx);

bool axion_suggest_tail_collapse(hvm::HVMContext* ctx);

} // extern "C"

