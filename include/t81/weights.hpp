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
    uint64_t trits = 0;
    uint64_t num_trits() const {
        return trits != 0 ? trits : padded_limbs() * 48;
    }
    uint64_t padded_limbs() const { return data.size(); }
};

using NativeModel = std::map<std::string, NativeTensor>;

struct ModelFile {
    std::vector<TensorInfo> tensors;
    uint64_t total_trits = 0;
    uint64_t total_parameters = 0;
    NativeModel native;
    std::string checksum;
    std::string format;
    uint64_t file_size = 0;
    double bits_per_trit = 0.0;
    double sparsity = 0.0;
};

void quantize_safetensors_to_gguf(const std::filesystem::path& input,
                                   const std::filesystem::path& output);

ModelFile load_gguf(const std::filesystem::path& path);
ModelFile load_safetensors(const std::filesystem::path& path);

NativeTensor import_bitnet_b158(std::span<const int8_t> src,
                                const std::vector<uint64_t>& shape);

void save_t81w(const NativeModel& model, const std::filesystem::path& path);
ModelFile load_t81w(const std::filesystem::path& path);

std::string emit_t81w_module(const ModelFile& mf, const std::string& source_name);

void print_info(const ModelFile& mf);
std::string format_bytes(uint64_t bytes);
std::string format_count(uint64_t value);

struct JsonValue {
    bool is_string = false;
    bool is_number = false;
    std::string string_value;
    double number_value = 0.0;
    std::vector<JsonValue> array_value;
    std::map<std::string, JsonValue> object_value;

    static JsonValue make_string(std::string s) {
        JsonValue v;
        v.is_string = true;
        v.string_value = std::move(s);
        return v;
    }

    static JsonValue make_number(double n) {
        JsonValue v;
        v.is_number = true;
        v.number_value = n;
        return v;
    }

    static JsonValue make_array(std::vector<JsonValue> a) {
        JsonValue v;
        v.array_value = std::move(a);
        return v;
    }

    static JsonValue make_object(std::map<std::string, JsonValue> o) {
        JsonValue v;
        v.object_value = std::move(o);
        return v;
    }
};

} // namespace t81::weights
