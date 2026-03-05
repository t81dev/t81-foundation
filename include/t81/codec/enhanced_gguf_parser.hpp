#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>

namespace t81::codec {

// GGUF data types (from GGUF specification)
enum class GGUFValueType : uint32_t {
    UINT8 = 0,
    INT8 = 1,
    UINT16 = 2,
    INT16 = 3,
    UINT32 = 4,
    INT32 = 5,
    FLOAT32 = 6,
    UINT64 = 7,
    INT64 = 8,
    FLOAT64 = 9,
    BOOL = 10,
    STRING = 11,
    ARRAY = 12,
    T3_K_QUANTIZED = 1000,  // Custom T81 ternary quantization
    BASE81_QUANTIZED = 1001 // Custom T81 base-81 quantization
};

// Enhanced tensor information
struct GGUTensorInfo {
    std::string name;
    std::vector<uint32_t> dimensions;
    uint64_t offset;
    GGUFValueType type;
    size_t element_count;
    size_t byte_size;
    bool is_quantized;
    
    // T81-specific fields
    std::optional<GGUFValueType> original_type;  // Before quantization
    std::optional<float> quantization_scale;
    std::optional<int32_t> quantization_zero_point;
};

// Model metadata
struct GGUFMetadata {
    uint32_t magic;
    uint32_t version;
    uint32_t tensor_count;
    uint32_t kv_count;
    std::unordered_map<std::string, std::string> general_metadata;
    std::unordered_map<std::string, std::vector<float>> float_arrays;
    std::unordered_map<std::string, std::vector<int32_t>> int_arrays;
};

// Memory management for tensor data
class TensorMemoryPool {
public:
    TensorMemoryPool(size_t initial_capacity = 1024 * 1024 * 1024); // 1GB default
    ~TensorMemoryPool();
    
    // Allocate memory for tensor data
    std::shared_ptr<uint8_t> allocate(size_t size);
    
    // Release memory back to pool
    void deallocate(void* ptr);
    
    // Get pool statistics
    size_t total_capacity() const { return total_capacity_; }
    size_t used_capacity() const { return used_capacity_; }
    size_t free_capacity() const { return total_capacity_ - used_capacity_; }
    
    // Compact pool (defragment)
    void compact();
    
private:
    struct MemoryBlock {
        uint8_t* data;
        size_t size;
        bool in_use;
        std::shared_ptr<uint8_t> shared_ptr;
    };
    
    std::vector<MemoryBlock> blocks_;
    size_t total_capacity_;
    size_t used_capacity_;
    mutable std::mutex mutex_;
};

// Enhanced GGUF parser with real tensor processing
class EnhancedGGUFParser {
public:
    explicit EnhancedGGUFParser(std::shared_ptr<TensorMemoryPool> memory_pool = nullptr);
    ~EnhancedGGUFParser() = default;
    
    // Parse GGUF file header and metadata
    std::optional<GGUFMetadata> parse_header(const std::string& file_path);
    
    // Parse tensor information only (no data loading)
    std::optional<std::vector<GGUTensorInfo>> parse_tensor_info(const std::string& file_path);
    
    // Load specific tensor data
    std::optional<std::vector<float>> load_tensor_float(const std::string& file_path, 
                                                       const GGUTensorInfo& tensor_info);
    
    // Load tensor data as quantized
    std::optional<std::vector<int8_t>> load_tensor_quantized(const std::string& file_path,
                                                            const GGUTensorInfo& tensor_info);
    
    // Validate tensor integrity
    bool validate_tensor(const std::string& file_path, const GGUTensorInfo& tensor_info);
    
    // Get memory usage statistics
    struct MemoryStats {
        size_t total_tensors_loaded = 0;
        size_t total_memory_used = 0;
        size_t quantized_memory_saved = 0;
        float compression_ratio = 0.0f;
    };
    MemoryStats get_memory_stats() const;
    
    // Clear cached tensor data
    void clear_cache();
    
    // Set memory pool
    void set_memory_pool(std::shared_ptr<TensorMemoryPool> pool) { memory_pool_ = pool; }
    
private:
    // Internal file reading helpers
    bool read_header(std::ifstream& file, GGUFMetadata& metadata);
    bool read_kv_pairs(std::ifstream& file, const GGUFMetadata& metadata);
    bool read_tensor_infos(std::ifstream& file, const GGUFMetadata& metadata,
                          std::vector<GGUTensorInfo>& tensor_infos);
    
    // Tensor data loading helpers
    template<typename T>
    std::optional<std::vector<T>> load_tensor_data(const std::string& file_path,
                                                   const GGUTensorInfo& tensor_info);
    
    // Quantization helpers
    std::vector<int8_t> quantize_to_ternary(const std::vector<float>& data);
    std::vector<float> dequantize_from_ternary(const std::vector<int8_t>& data);
    
    // Cache management
    struct CachedTensor {
        std::vector<float> float_data;
        std::vector<int8_t> quantized_data;
        bool is_quantized;
        size_t access_count;
        std::chrono::steady_clock::time_point last_access;
    };
    
    std::unordered_map<std::string, CachedTensor> tensor_cache_;
    std::shared_ptr<TensorMemoryPool> memory_pool_;
    MemoryStats memory_stats_;
    mutable std::mutex cache_mutex_;
    
    // Cache policy
    static constexpr size_t MAX_CACHE_SIZE = 100;
    static constexpr size_t MAX_CACHE_MEMORY = 512 * 1024 * 1024; // 512MB
    
    void evict_from_cache_if_needed();
    void update_cache_access(const std::string& tensor_name);
};

// Utility functions
namespace gguf_utils {
    // Check if file is valid GGUF
    bool is_valid_gguf(const std::string& file_path);
    
    // Get file size
    size_t get_file_size(const std::string& file_path);
    
    // Calculate tensor memory requirements
    size_t calculate_tensor_size(const GGUTensorInfo& tensor);
    
    // Validate tensor dimensions
    bool validate_tensor_dimensions(const std::vector<uint32_t>& dims);
    
    // Convert GGUF type to string
    std::string type_to_string(GGUFValueType type);
    
    // Get quantization ratio
    float get_quantization_ratio(GGUFValueType original_type, GGUFValueType quantized_type);
}

} // namespace t81::codec
