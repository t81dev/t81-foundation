#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <optional>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "t81/ternary.hpp"

namespace t81::codec {

// Advanced ternary quantization types
enum class TernaryType : int8_t {
    NEGATIVE = -1,
    ZERO = 0,
    POSITIVE = 1
};

// Quantization strategies
enum class QuantizationStrategy {
    THRESHOLD,     // Simple threshold-based quantization
    PERCENTILE,    // Percentile-based quantization
    K_MEANS,       // K-means clustering (3 clusters)
    ADAPTIVE,      // Adaptive threshold based on data distribution
    ENTROPY_MIN    // Minimize entropy for optimal compression
};

// T3_K quantization configuration
struct T3KConfig {
    QuantizationStrategy strategy = QuantizationStrategy::ADAPTIVE;
    float threshold_positive = 0.5f;
    float threshold_negative = -0.5f;
    float percentile_high = 75.0f;
    float percentile_low = 25.0f;
    bool preserve_sparsity = true;
    bool optimize_for_inference = true;
    int max_iterations = 100;
    float convergence_tolerance = 1e-6f;
};

// Quantization statistics
struct QuantizationStats {
    size_t total_elements = 0;
    size_t positive_count = 0;
    size_t negative_count = 0;
    size_t zero_count = 0;
    float mean_squared_error = 0.0f;
    float signal_to_noise_ratio = 0.0f;
    float compression_ratio = 0.0f;
    float sparsity_ratio = 0.0f;
    float entropy_bits = 0.0f;
};

// Advanced T3_K Quantizer
class AdvancedT3KQuantizer {
public:
    explicit AdvancedT3KQuantizer(const T3KConfig& config = T3KConfig{});
    ~AdvancedT3KQuantizer() = default;
    
    // Main quantization methods
    std::vector<int8_t> quantize(const std::vector<float>& input);
    std::vector<float> dequantize(const std::vector<int8_t>& input);
    
    // Batch processing
    std::vector<std::vector<int8_t>> quantize_batch(const std::vector<std::vector<float>>& inputs);
    std::vector<std::vector<float>> dequantize_batch(const std::vector<std::vector<int8_t>>& inputs);
    
    // Adaptive quantization (learns optimal thresholds from data)
    void learn_from_data(const std::vector<float>& training_data);
    void learn_from_batch(const std::vector<std::vector<float>>& training_batch);
    
    // Statistics and analysis
    QuantizationStats get_last_stats() const { return last_stats_; }
    void analyze_distribution(const std::vector<float>& data);
    
    // Configuration
    void set_config(const T3KConfig& config) { config_ = config; }
    const T3KConfig& get_config() const { return config_; }
    
    // Utility methods
    static float calculate_mse(const std::vector<float>& original, const std::vector<float>& quantized);
    static float calculate_snr(const std::vector<float>& original, const std::vector<float>& quantized);
    static float calculate_entropy(const std::vector<int8_t>& data);
    
private:
    // Quantization strategies implementation
    std::vector<int8_t> quantize_threshold(const std::vector<float>& input);
    std::vector<int8_t> quantize_percentile(const std::vector<float>& input);
    std::vector<int8_t> quantize_kmeans(const std::vector<float>& input);
    std::vector<int8_t> quantize_adaptive(const std::vector<float>& input);
    std::vector<int8_t> quantize_entropy_min(const std::vector<float>& input);
    
    // K-means clustering helpers
    struct KMeansCluster {
        float centroid;
        std::vector<float> points;
    };
    std::vector<KMeansCluster> kmeans_cluster(const std::vector<float>& data, int k);
    
    // Adaptive threshold calculation
    void calculate_adaptive_thresholds(const std::vector<float>& data);
    
    // Entropy minimization
    std::vector<float> find_optimal_thresholds_entropy(const std::vector<float>& data);
    
    // Data analysis
    std::vector<float> calculate_percentiles(const std::vector<float>& data, 
                                            const std::vector<float>& percentiles);
    
    T3KConfig config_;
    QuantizationStats last_stats_;
    bool thresholds_calculated_ = false;
};

// Ternary Tensor Operations
class TernaryTensorOps {
public:
    // Basic operations
    static std::vector<int8_t> add(const std::vector<int8_t>& a, const std::vector<int8_t>& b);
    static std::vector<int8_t> multiply(const std::vector<int8_t>& a, const std::vector<int8_t>& b);
    static std::vector<int8_t> scale(const std::vector<int8_t>& input, int8_t scalar);
    
    // Matrix operations for neural networks
    static std::vector<int8_t> matmul_ternary(const std::vector<int8_t>& weights, 
                                             const std::vector<int8_t>& input,
                                             size_t input_size, size_t output_size);
    
    // Convolution operations
    static std::vector<int8_t> conv2d_ternary(const std::vector<int8_t>& input,
                                             const std::vector<int8_t>& kernel,
                                             size_t input_height, size_t input_width,
                                             size_t kernel_height, size_t kernel_width,
                                             size_t channels);
    
    // Pooling operations
    static std::vector<int8_t> max_pool_ternary(const std::vector<int8_t>& input,
                                               size_t height, size_t width,
                                               size_t pool_size, size_t stride);
    
    // Activation functions
    static std::vector<int8_t> ternary_relu(const std::vector<int8_t>& input);
    static std::vector<int8_t> ternary_tanh(const std::vector<int8_t>& input);
    
    // Utility functions
    static size_t count_ones(const std::vector<int8_t>& input);
    static size_t count_neg_ones(const std::vector<int8_t>& input);
    static size_t count_zeros(const std::vector<int8_t>& input);
    static float calculate_density(const std::vector<int8_t>& input);
};

// Memory-efficient ternary storage
class PackedTernaryStorage {
public:
    // Pack 3 ternary values into 2 bytes (5 bits per value, with 1 bit unused)
    static std::vector<uint8_t> pack_ternary(const std::vector<int8_t>& input);
    static std::vector<int8_t> unpack_ternary(const std::vector<uint8_t>& packed);
    
    // Calculate storage requirements
    static size_t packed_size(size_t original_size) { return (original_size * 5 + 7) / 8; }
    static float compression_ratio() { return 3.0f / 1.25f; } // 3 values in 1.25 bytes
    
    // Bit manipulation helpers
    static uint8_t pack_three_values(int8_t v1, int8_t v2, int8_t v3);
    static std::tuple<int8_t, int8_t, int8_t> unpack_three_values(uint8_t packed);
};

// Ternary quantization cache for frequently used tensors
class TernaryQuantizationCache {
public:
    TernaryQuantizationCache(size_t max_size = 1000);
    ~TernaryQuantizationCache() = default;
    
    // Cache operations
    void put(const std::string& key, const std::vector<int8_t>& quantized_data);
    std::optional<std::vector<int8_t>> get(const std::string& key);
    void remove(const std::string& key);
    void clear();
    
    // Cache statistics
    size_t size() const;
    size_t memory_usage() const;
    float hit_rate() const;
    
private:
    struct CacheEntry {
        std::vector<int8_t> data;
        std::chrono::steady_clock::time_point last_access;
        size_t access_count;
    };
    
    std::unordered_map<std::string, CacheEntry> cache_;
    size_t max_size_;
    mutable size_t total_accesses_ = 0;
    mutable size_t cache_hits_ = 0;
    
    void evict_lru_if_needed();
};

// Utility functions
namespace ternary_utils {
    // Convert between different ternary representations
    std::vector<TernaryType> to_ternary_type(const std::vector<int8_t>& input);
    std::vector<int8_t> from_ternary_type(const std::vector<TernaryType>& input);
    
    // Validate ternary data
    bool is_valid_ternary(const std::vector<int8_t>& input);
    bool is_valid_ternary(const std::vector<TernaryType>& input);
    
    // Convert to/from T81 native ternary format
    std::vector<t81::Ternary> to_t81_ternary(const std::vector<int8_t>& input);
    std::vector<int8_t> from_t81_ternary(const std::vector<t81::Ternary>& input);
    
    // Statistical analysis
    float calculate_ternary_balance(const std::vector<int8_t>& input);
    std::vector<float> calculate_transition_probabilities(const std::vector<int8_t>& input);
}

} // namespace t81::codec
