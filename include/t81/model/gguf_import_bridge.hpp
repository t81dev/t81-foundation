#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "t81/support/expected.hpp"

namespace t81::model {

struct GgufTensorDescriptor {
  std::string name;
  std::vector<uint64_t> shape;
  std::string source_type;
  uint64_t element_count = 0;
  uint64_t byte_size = 0;
  bool quantized = false;
};

class GgufImportBridge {
public:
  static t81::expected<std::unique_ptr<GgufImportBridge>, std::string> open(
      const std::filesystem::path& model_path);

  virtual ~GgufImportBridge() = default;

  virtual std::string model_architecture() const = 0;
  virtual std::vector<GgufTensorDescriptor> list_tensors() const = 0;
  virtual t81::expected<std::vector<float>, std::string> read_tensor_f32(
      std::string_view tensor_name) = 0;
};

}  // namespace t81::model
