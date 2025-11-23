#include "axion_api.hpp"

#include <algorithm>  // std::min
#include <cstring>    // std::strncpy
#include <cmath>      // std::fmin

namespace axion {

std::string frame_optimize(const hvm::HVMContext& ctx) {
    if (ctx.stack_ptr == 0) {
        return "Base case detected";
    }

    // Note: assumes stack_ptr is a valid index into ctx.stack.
    const auto& frame = ctx.stack[ctx.stack_ptr];

    if (frame.return_addr == ctx.pc) {
        return "Tail recursion detected";
    }

    return "Standard recursive call";
}

double predict_score(const hvm::HVMContext& ctx) {
    // Normalized recursion depth; 729.0 is taken from your C code.
    const double normalized_depth =
        static_cast<double>(ctx.stack_ptr) / 729.0;

    // 1.0 at shallow depth; 0.0 at or beyond depth 729.
    return 1.0 - std::fmin(1.0, normalized_depth);
}

bool suggest_tail_collapse(const hvm::HVMContext& ctx) {
    if (ctx.stack_ptr == 0) {
        return false;
    }

    const auto& frame = ctx.stack[ctx.stack_ptr];
    return frame.return_addr == ctx.pc;
}

} // namespace axion

// --- C-style wrappers --------------------------------------------------------

extern "C" {

void axion_frame_optimize(hvm::HVMContext* ctx,
                          char* out_annotation,
                          std::size_t max_len) {
    if (!ctx || !out_annotation || max_len == 0) {
        return;
    }

    const std::string msg = axion::frame_optimize(*ctx);

    // Preserve original semantics: truncate without guaranteeing null-termination
    // if max_len is too small. If you want strict safety, force null-termination.
    std::strncpy(out_annotation, msg.c_str(), max_len);

    if (max_len > 0) {
        out_annotation[max_len - 1] = '\0';
    }
}

double axion_predict_score(hvm::HVMContext* ctx) {
    if (!ctx) {
        return 0.0;
    }
    return axion::predict_score(*ctx);
}

bool axion_suggest_tail_collapse(hvm::HVMContext* ctx) {
    if (!ctx) {
        return false;
    }
    return axion::suggest_tail_collapse(*ctx);
}

} // extern "C"

