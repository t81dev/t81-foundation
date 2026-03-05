// Ternary GGUF Header
// Created for T81 LLM integration

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace t81::codec {

// GGUF tensor types for ternary quantization
enum class GGUFDataType {
    T3K_QUANTIZED = 1000,
    BASE81_QUANTIZED = 1001,
    MIXED_TERNARY = 1002
};

// Ternary GGUF reader/writer
class TernaryGGUF {
public:
    static bool read_ternary_tensor(const std::string& filename, 
                                   std::vector<float>& output,
                                   GGUFDataType& type);
    static bool write_ternary_tensor(const std::string& filename,
                                    const std::vector<float>& input,
                                    GGUFDataType type);
    
    // Utility functions
    static bool is_ternary_compatible(const std::string& gguf_file);
    static size_t calculate_compression_ratio(size_t original_size, size_t compressed_size);
};

} // namespace t81::codec
