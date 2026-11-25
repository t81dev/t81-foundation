#include "metal_dispatch.hpp"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <cstring>
#include <mutex>

namespace t81::bench::h06_mem_04 {
namespace {

static const char* kMetalKernelSource = R"(
#include <metal_stdlib>
using namespace metal;

struct MatmulParams {
  uint batch;
  uint out_dim;
  uint k_dim;
};

kernel void ternary_batch_matmul_i64(
    device const char* weights     [[buffer(0)]],
    device const char* activations [[buffer(1)]],
    device long*       output      [[buffer(2)]],
    constant MatmulParams& p       [[buffer(3)]],
    uint2 gid                      [[thread_position_in_grid]]) {
  const uint o = gid.x;
  const uint b = gid.y;
  if (o >= p.out_dim || b >= p.batch) {
    return;
  }
  const uint w_off = o * p.k_dim;
  const uint a_off = b * p.k_dim;
  long acc = 0;
  for (uint k = 0; k < p.k_dim; ++k) {
    acc += static_cast<long>(activations[a_off + k]) * static_cast<long>(weights[w_off + k]);
  }
  output[b * p.out_dim + o] = acc;
}
)";

struct MetalContext {
  id<MTLDevice> device = nil;
  id<MTLCommandQueue> queue = nil;
  id<MTLComputePipelineState> pipeline = nil;
  bool initialized = false;
};

MetalContext& get_metal_context() {
  static MetalContext ctx;
  static std::once_flag once;
  std::call_once(once, []() {
    @autoreleasepool {
      ctx.device = MTLCreateSystemDefaultDevice();
      if (!ctx.device) return;
      ctx.queue = [ctx.device newCommandQueue];
      if (!ctx.queue) return;

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kMetalKernelSource];
      id<MTLLibrary> lib = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (!lib || err != nil) return;

      id<MTLFunction> fn = [lib newFunctionWithName:@"ternary_batch_matmul_i64"];
      if (!fn) return;

      ctx.pipeline = [ctx.device newComputePipelineStateWithFunction:fn error:&err];
      if (!ctx.pipeline || err != nil) return;

      ctx.initialized = true;
    }
  });
  return ctx;
}

struct MatmulParamsHost {
  std::uint32_t batch;
  std::uint32_t out_dim;
  std::uint32_t k_dim;
};

}  // namespace

bool metal_backend_available() noexcept {
  return get_metal_context().initialized;
}

bool metal_execute_ternary_matmul(const std::int8_t* weights,
                                  const std::int8_t* activations,
                                  std::int64_t* output,
                                  std::uint32_t batch,
                                  std::uint32_t out_dim,
                                  std::uint32_t k_dim,
                                  std::uint32_t queue_depth,
                                  std::uint32_t max_queue_depth) noexcept {
  if (queue_depth > max_queue_depth) {
    return false;
  }
  MetalContext& ctx = get_metal_context();
  if (!ctx.initialized) {
    return false;
  }

  @autoreleasepool {
    const std::size_t w_bytes = static_cast<std::size_t>(out_dim) * k_dim * sizeof(std::int8_t);
    const std::size_t a_bytes = static_cast<std::size_t>(batch) * k_dim * sizeof(std::int8_t);
    const std::size_t o_bytes = static_cast<std::size_t>(batch) * out_dim * sizeof(std::int64_t);

    id<MTLBuffer> w_buf = [ctx.device newBufferWithBytes:weights
                                                  length:w_bytes
                                                 options:MTLResourceStorageModeShared];
    id<MTLBuffer> a_buf = [ctx.device newBufferWithBytes:activations
                                                  length:a_bytes
                                                 options:MTLResourceStorageModeShared];
    id<MTLBuffer> o_buf = [ctx.device newBufferWithLength:o_bytes
                                                  options:MTLResourceStorageModeShared];
    if (!w_buf || !a_buf || !o_buf) {
      return false;
    }

    MatmulParamsHost params{batch, out_dim, k_dim};
    id<MTLCommandBuffer> cmd = [ctx.queue commandBuffer];
    if (!cmd) {
      return false;
    }

    id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
    if (!enc) {
      return false;
    }
    [enc setComputePipelineState:ctx.pipeline];
    [enc setBuffer:w_buf offset:0 atIndex:0];
    [enc setBuffer:a_buf offset:0 atIndex:1];
    [enc setBuffer:o_buf offset:0 atIndex:2];
    [enc setBytes:&params length:sizeof(params) atIndex:3];

    const NSUInteger tg_w = std::min<NSUInteger>(16, out_dim);
    const NSUInteger tg_h = std::min<NSUInteger>(16, batch);
    MTLSize grid = MTLSizeMake(out_dim, batch, 1);
    MTLSize tg = MTLSizeMake(std::max<NSUInteger>(1, tg_w), std::max<NSUInteger>(1, tg_h), 1);
    [enc dispatchThreads:grid threadsPerThreadgroup:tg];
    [enc endEncoding];

    [cmd commit];
    [cmd waitUntilCompleted];
    if ([cmd status] != MTLCommandBufferStatusCompleted) {
      return false;
    }

    std::memcpy(output, [o_buf contents], o_bytes);
    return true;
  }
}

}  // namespace t81::bench::h06_mem_04
