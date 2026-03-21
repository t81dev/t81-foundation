#include "t81/model/gguf_import_bridge.hpp"

#include <filesystem>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(T81_HAS_LLAMA_CPP)
#include <ggml.h>
#include <gguf.h>
#endif

namespace t81::model {

namespace {

constexpr std::string_view kGgufImportBridgeRevision = "llama.cpp-bridge-v1";

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

std::string ggml_type_to_string(enum ggml_type type) {
  const char* name = ggml_type_name(type);
  return name != nullptr ? std::string(name) : std::string("unknown");
}

bool ggml_type_is_quantized(enum ggml_type type) { return ggml_is_quantized(type); }

uint64_t tensor_element_count(const ggml_tensor* tensor) {
  uint64_t total = 1;
  const int dims = ggml_n_dims(tensor);
  for (int i = 0; i < dims; ++i) {
    total *= static_cast<uint64_t>(tensor->ne[i]);
  }
  return total;
}

t81::expected<std::pair<std::unique_ptr<gguf_context, GgufContextDeleter>,
                        std::unique_ptr<ggml_context, GgmlContextDeleter>>,
              std::string>
open_gguf_context(const std::filesystem::path& model_path, bool no_alloc) {
  ggml_context* tensor_ctx_raw = nullptr;
  gguf_init_params params{};
  params.no_alloc = no_alloc;
  params.ctx = &tensor_ctx_raw;

  std::unique_ptr<gguf_context, GgufContextDeleter> gguf_ctx(
      gguf_init_from_file(model_path.string().c_str(), params));
  std::unique_ptr<ggml_context, GgmlContextDeleter> tensor_ctx(tensor_ctx_raw);
  if (gguf_ctx == nullptr || tensor_ctx == nullptr) {
    return t81::make_unexpected("failed to open GGUF metadata via llama.cpp: " +
                                model_path.string());
  }
  return std::make_pair(std::move(gguf_ctx), std::move(tensor_ctx));
}

ggml_tensor* find_tensor_by_name(ggml_context* ctx, std::string_view tensor_name) {
  for (ggml_tensor* tensor = ggml_get_first_tensor(ctx); tensor != nullptr;
       tensor = ggml_get_next_tensor(ctx, tensor)) {
    if (tensor_name == ggml_get_name(tensor)) {
      return tensor;
    }
  }
  return nullptr;
}

std::vector<float> dequantize_tensor_to_f32(ggml_tensor* tensor) {
  const uint64_t nelements = tensor_element_count(tensor);
  std::vector<float> out(static_cast<size_t>(nelements));

  if (tensor->type == GGML_TYPE_F32) {
    std::memcpy(out.data(), tensor->data, out.size() * sizeof(float));
    return out;
  }

  if (tensor->type == GGML_TYPE_F16) {
    ggml_fp16_to_fp32_row(reinterpret_cast<const ggml_fp16_t*>(tensor->data), out.data(),
                          static_cast<int64_t>(nelements));
    return out;
  }

  if (tensor->type == GGML_TYPE_BF16) {
    ggml_bf16_to_fp32_row(reinterpret_cast<const ggml_bf16_t*>(tensor->data), out.data(),
                          static_cast<int64_t>(nelements));
    return out;
  }

  const ggml_type_traits* traits = ggml_get_type_traits(tensor->type);
  if (!ggml_is_quantized(tensor->type) || traits == nullptr || traits->to_float == nullptr) {
    throw std::runtime_error("cannot export tensor type " + ggml_type_to_string(tensor->type) +
                             " to float32");
  }

  traits->to_float(tensor->data, out.data(), static_cast<int64_t>(nelements));
  return out;
}

std::vector<uint64_t> tensor_shape(const ggml_tensor* tensor) {
  std::vector<uint64_t> shape;
  const int dims = ggml_n_dims(tensor);
  shape.reserve(static_cast<size_t>(dims));
  for (int i = 0; i < dims; ++i) {
    shape.push_back(static_cast<uint64_t>(tensor->ne[i]));
  }
  return shape;
}

class LlamaCppGgufImportBridge final : public GgufImportBridge {
public:
  static t81::expected<std::unique_ptr<GgufImportBridge>, std::string> open_bridge(
      const std::filesystem::path& model_path) {
    if (!std::filesystem::exists(model_path)) {
      return t81::make_unexpected("model file not found: " + model_path.string());
    }

    auto opened = open_gguf_context(model_path, true);
    if (!opened.has_value()) {
      return t81::make_unexpected(opened.error());
    }
    auto [gguf_ctx, tensor_ctx] = std::move(opened.value());

    std::string architecture = "unknown";
    const int64_t arch_key = gguf_find_key(gguf_ctx.get(), "general.architecture");
    if (arch_key >= 0 && gguf_get_kv_type(gguf_ctx.get(), arch_key) == GGUF_TYPE_STRING) {
      const char* value = gguf_get_val_str(gguf_ctx.get(), arch_key);
      if (value != nullptr) {
        architecture = value;
      }
    }

    std::vector<GgufTensorDescriptor> tensors;
    const int64_t n_tensors = gguf_get_n_tensors(gguf_ctx.get());
    tensors.reserve(static_cast<size_t>(n_tensors));
    for (ggml_tensor* tensor = ggml_get_first_tensor(tensor_ctx.get()); tensor != nullptr;
         tensor = ggml_get_next_tensor(tensor_ctx.get(), tensor)) {
      GgufTensorDescriptor desc;
      desc.name = ggml_get_name(tensor);
      desc.shape = tensor_shape(tensor);
      desc.source_type = ggml_type_to_string(tensor->type);
      desc.element_count = tensor_element_count(tensor);
      desc.byte_size = static_cast<uint64_t>(ggml_nbytes(tensor));
      desc.quantized = ggml_type_is_quantized(tensor->type);
      tensors.push_back(std::move(desc));
    }

    auto bridge = std::unique_ptr<LlamaCppGgufImportBridge>(new LlamaCppGgufImportBridge());
    bridge->model_path_ = model_path;
    bridge->architecture_ = std::move(architecture);
    bridge->tensors_ = std::move(tensors);
    return std::unique_ptr<GgufImportBridge>(std::move(bridge));
  }

  std::string model_architecture() const override { return architecture_; }

  std::vector<GgufTensorDescriptor> list_tensors() const override { return tensors_; }

  t81::expected<std::vector<float>, std::string> read_tensor_f32(
      std::string_view tensor_name) override {
    auto opened = open_gguf_context(model_path_, false);
    if (!opened.has_value()) {
      return t81::make_unexpected(opened.error());
    }
    auto [gguf_ctx, tensor_ctx] = std::move(opened.value());
    (void)gguf_ctx;

    ggml_tensor* tensor = find_tensor_by_name(tensor_ctx.get(), tensor_name);
    if (tensor == nullptr) {
      return t81::make_unexpected("tensor not found in GGUF: " + std::string(tensor_name));
    }

    try {
      return dequantize_tensor_to_f32(tensor);
    } catch (const std::exception& ex) {
      return t81::make_unexpected(ex.what());
    }
  }

private:
  std::filesystem::path model_path_;
  std::string architecture_;
  std::vector<GgufTensorDescriptor> tensors_;
};

#endif

}  // namespace

t81::expected<std::unique_ptr<GgufImportBridge>, std::string> GgufImportBridge::open(
    const std::filesystem::path& model_path) {
#if defined(T81_HAS_LLAMA_CPP)
  return LlamaCppGgufImportBridge::open_bridge(model_path);
#else
  (void)model_path;
  return t81::make_unexpected(
      "GGUF import bridge is unavailable in this build (reconfigure with -DT81_ENABLE_LLAMA_CPP=ON)");
#endif
}

std::string GgufImportBridge::bridge_revision() {
  return std::string(kGgufImportBridgeRevision);
}

}  // namespace t81::model
