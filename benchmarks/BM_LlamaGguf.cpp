#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#if defined(T81_HAS_LLAMA_CPP)
#include <ggml.h>
#include <gguf.h>
#endif

namespace {

#if defined(T81_HAS_LLAMA_CPP)

struct GgufContextDeleter {
  void operator()(gguf_context* ctx) const noexcept {
    if (ctx != nullptr) {
      gguf_free(ctx);
    }
  }
};

struct GgmlContextDeleter {
  void operator()(ggml_context* ctx) const noexcept {
    if (ctx != nullptr) {
      ggml_free(ctx);
    }
  }
};

struct QuantizedTensorFixture {
  std::unique_ptr<gguf_context, GgufContextDeleter> gguf_ctx;
  std::unique_ptr<ggml_context, GgmlContextDeleter> tensor_ctx;
  ggml_tensor* tensor = nullptr;
  std::vector<float> scratch;
  std::string model_name;
  std::string tensor_name;
  std::string tensor_type;
  uint64_t element_count = 0;
};

uint64_t tensor_element_count(const ggml_tensor* tensor) {
  uint64_t total = 1;
  const int dims = ggml_n_dims(tensor);
  for (int i = 0; i < dims; ++i) {
    total *= static_cast<uint64_t>(tensor->ne[i]);
  }
  return total;
}

std::filesystem::path find_repo_model_path() {
  namespace fs = std::filesystem;
  const fs::path model_dir = fs::current_path() / "models";
  const std::vector<std::string> preferred = {
      "tinyllama-1.1b.Q2_K.gguf",
      "Phi-3-mini-4k-instruct-q4.gguf",
      "Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf",
  };
  for (const auto& candidate : preferred) {
    const fs::path path = model_dir / candidate;
    if (fs::exists(path)) {
      return path;
    }
  }
  if (fs::exists(model_dir)) {
    for (const auto& entry : fs::directory_iterator(model_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
        return entry.path();
      }
    }
  }
  return {};
}

std::unique_ptr<QuantizedTensorFixture> load_quantized_tensor_fixture(std::string* error) {
  namespace fs = std::filesystem;
  const fs::path model_path = find_repo_model_path();
  if (model_path.empty()) {
    if (error) *error = "no GGUF model found under models/";
    return nullptr;
  }

  ggml_context* tensor_ctx_raw = nullptr;
  gguf_init_params params{};
  params.no_alloc = false;
  params.ctx = &tensor_ctx_raw;

  auto gguf_ctx = std::unique_ptr<gguf_context, GgufContextDeleter>(
      gguf_init_from_file(model_path.string().c_str(), params));
  auto tensor_ctx = std::unique_ptr<ggml_context, GgmlContextDeleter>(tensor_ctx_raw);
  if (gguf_ctx == nullptr || tensor_ctx == nullptr) {
    if (error) *error = "failed to open GGUF via llama.cpp: " + model_path.string();
    return nullptr;
  }

  ggml_tensor* best_tensor = nullptr;
  uint64_t best_size = 0;
  for (ggml_tensor* tensor = ggml_get_first_tensor(tensor_ctx.get()); tensor != nullptr;
       tensor = ggml_get_next_tensor(tensor_ctx.get(), tensor)) {
    if (!ggml_is_quantized(tensor->type)) {
      continue;
    }
    const uint64_t count = tensor_element_count(tensor);
    if (count < 64) {
      continue;
    }
    if (best_tensor == nullptr || count < best_size) {
      best_tensor = tensor;
      best_size = count;
    }
  }
  if (best_tensor == nullptr) {
    if (error) *error = "no quantized tensor found in GGUF model: " + model_path.string();
    return nullptr;
  }

  auto fixture = std::make_unique<QuantizedTensorFixture>();
  fixture->element_count = tensor_element_count(best_tensor);
  fixture->scratch.resize(static_cast<size_t>(fixture->element_count));
  fixture->model_name = model_path.filename().string();
  fixture->tensor_name = ggml_get_name(best_tensor);
  fixture->tensor_type = ggml_type_name(best_tensor->type);
  fixture->tensor = best_tensor;
  fixture->gguf_ctx = std::move(gguf_ctx);
  fixture->tensor_ctx = std::move(tensor_ctx);
  return fixture;
}

bool dequantize_tensor_to_f32(const ggml_tensor* tensor, float* out, uint64_t nelements) {
  if (tensor->type == GGML_TYPE_F32) {
    std::memcpy(out, tensor->data, static_cast<size_t>(nelements) * sizeof(float));
    return true;
  }
  if (tensor->type == GGML_TYPE_F16) {
    ggml_fp16_to_fp32_row(reinterpret_cast<const ggml_fp16_t*>(tensor->data), out,
                         static_cast<int64_t>(nelements));
    return true;
  }
  if (tensor->type == GGML_TYPE_BF16) {
    ggml_bf16_to_fp32_row(reinterpret_cast<const ggml_bf16_t*>(tensor->data), out,
                         static_cast<int64_t>(nelements));
    return true;
  }

  const ggml_type_traits* traits = ggml_get_type_traits(tensor->type);
  if (!ggml_is_quantized(tensor->type) || traits == nullptr || traits->to_float == nullptr) {
    return false;
  }
  traits->to_float(tensor->data, out, static_cast<int64_t>(nelements));
  return true;
}

const QuantizedTensorFixture& quantized_tensor_fixture(benchmark::State& state) {
  static std::unique_ptr<QuantizedTensorFixture> fixture;
  static std::string error_message;
  if (!fixture && error_message.empty()) {
    fixture = load_quantized_tensor_fixture(&error_message);
  }
  if (!fixture) {
    state.SkipWithError(error_message.c_str());
    static QuantizedTensorFixture empty;
    return empty;
  }
  return *fixture;
}

static void BM_LlamaGgufDequantize_Binary(benchmark::State& state) {
  const auto& fixture = quantized_tensor_fixture(state);
  if (fixture.tensor == nullptr) {
    return;
  }
  const uint64_t work_items =
      std::min<uint64_t>(static_cast<uint64_t>(state.range(0)), fixture.element_count);

  for (auto _ : state) {
    if (!dequantize_tensor_to_f32(fixture.tensor, const_cast<float*>(fixture.scratch.data()), work_items)) {
      state.SkipWithError("failed to dequantize tensor with llama.cpp");
      return;
    }
    benchmark::DoNotOptimize(fixture.scratch.data());
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(work_items));
  state.SetLabel("comparison=systems-path; model=" + fixture.model_name + "; tensor=" +
                  fixture.tensor_type + "; work: ops/iter=" +
                 std::to_string(work_items));
}
BENCHMARK(BM_LlamaGgufDequantize_Binary)->Arg(8192)->Arg(65536);

static void BM_LlamaGgufDequantizeAndExp_Binary(benchmark::State& state) {
  const auto& fixture = quantized_tensor_fixture(state);
  if (fixture.tensor == nullptr) {
    return;
  }
  const uint64_t work_items =
      std::min<uint64_t>(static_cast<uint64_t>(state.range(0)), fixture.element_count);

  for (auto _ : state) {
    auto* scratch = const_cast<float*>(fixture.scratch.data());
    if (!dequantize_tensor_to_f32(fixture.tensor, scratch, work_items)) {
      state.SkipWithError("failed to dequantize tensor with llama.cpp");
      return;
    }
    for (uint64_t i = 0; i < work_items; ++i) {
      scratch[i] = std::exp(scratch[i]);
    }
    benchmark::DoNotOptimize(scratch);
  }
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(work_items));
  state.SetLabel("comparison=systems-path; model=" + fixture.model_name + "; tensor=" +
                  fixture.tensor_type + "; work: ops/iter=" +
                 std::to_string(work_items));
}
BENCHMARK(BM_LlamaGgufDequantizeAndExp_Binary)->Arg(8192)->Arg(65536);

#else

static void BM_LlamaGgufDequantize_Binary(benchmark::State& state) {
  state.SkipWithError("llama.cpp benchmarks unavailable in this build");
}
BENCHMARK(BM_LlamaGgufDequantize_Binary);

static void BM_LlamaGgufDequantizeAndExp_Binary(benchmark::State& state) {
  state.SkipWithError("llama.cpp benchmarks unavailable in this build");
}
BENCHMARK(BM_LlamaGgufDequantizeAndExp_Binary);

#endif

}  // namespace
