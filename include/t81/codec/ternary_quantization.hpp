// Ternary Quantization Header
// Created for T81 LLM integration

#pragma once

#include <vector>
#include <cstdint>

namespace t81::codec {

// Ternary quantization types
enum class TernaryType {
    BALANCED = -1,
    ZERO = 0,
    POSITIVE = 1
};

// T3_K quantization (2.63-bit)
class T3KQuantizer {
public:
    static std::vector<int8_t> quantize(const std::vector<float>& input);
    static std::vector<float> dequantize(const std::vector<int8_t>& input);
    static constexpr float COMPRESSION_RATIO = 96.0f;
};

// Base-81 quantization
class Base81Quantizer {
public:
    static std::vector<int8_t> quantize_to_base81(const std::vector<float>& input);
    static std::vector<float> dequantize_from_base81(const std::vector<int8_t>& input);
};

} // namespace t81::codec
