#pragma once

#include "t81/core/tensor.hpp"
#include <cstdint>

#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace t81::weights {

struct TensorInfo {
    std::string name;
    std::vector<uint64_t> shape;
    uint64_t num_trits = 0;
    double sparsity = 0.0;
};

struct NativeTensor {
    std::vector<uint64_t> shape;
    std::vector<uint64_t> data;
    uint64_t num_trits() const { return data.size() * 48; }
};

using NativeModel = std::map<std::string, NativeTensor>;

struct ModelFile {
    std::vector<TensorInfo> tensors;
    uint64_t total_trits = 0;
    uint64_t total_parameters = 0;
    NativeModel native;
};

ModelFile load_gguf(const std::filesystem::path& path);
ModelFile load_safetensors(const std::filesystem::path& path);

NativeTensor import_bitnet_b158(std::span<const int8_t> src,
                                const std::vector<uint64_t>& shape);

void save_t81w(const NativeModel& model, const std::filesystem::path& path);
NativeModel load_t81w(const std::filesystem::path& path);

void print_info(const ModelFile& mf);
std::string format_bytes(uint64_t bytes);

} // namespace t81::weights
