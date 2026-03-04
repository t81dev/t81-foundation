#include "t81/codec/enhanced_gguf_parser.hpp"
#include "t81/codec/ternary_quantization.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <mutex>

namespace t81::codec {

// Constants for GGUF format
constexpr uint32_t GGUF_MAGIC = 0x46554747; // "GGUF" in little endian
constexpr uint32_t GGUF_VERSION = 3;

// TensorMemoryPool implementation
TensorMemoryPool::TensorMemoryPool(size_t initial_capacity) 
    : total_capacity_(initial_capacity), used_capacity_(0) {
    // Allocate initial block
    MemoryBlock block;
    block.data = new uint8_t[initial_capacity];
    block.size = initial_capacity;
    block.in_use = false;
    block.shared_ptr = std::shared_ptr<uint8_t>(block.data, [this](uint8_t* ptr) {
        // Custom deleter that returns memory to pool
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& b : blocks_) {
            if (b.data == ptr) {
                b.in_use = false;
                used_capacity_ -= b.size;
                break;
            }
        }
    });
    blocks_.push_back(std::move(block));
}

TensorMemoryPool::~TensorMemoryPool() {
    for (auto& block : blocks_) {
        delete[] block.data;
    }
}

std::shared_ptr<uint8_t> TensorMemoryPool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find a suitable free block
    for (auto& block : blocks_) {
        if (!block.in_use && block.size >= size) {
            block.in_use = true;
            used_capacity_ += size;
            return block.shared_ptr;
        }
    }
    
    // No suitable block found, allocate new one
    size_t new_block_size = std::max(size, total_capacity_ / 4); // At least 1/4 of current capacity
    MemoryBlock new_block;
    new_block.data = new uint8_t[new_block_size];
    new_block.size = new_block_size;
    new_block.in_use = true;
    new_block.shared_ptr = std::shared_ptr<uint8_t>(new_block.data, [this](uint8_t* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& b : blocks_) {
            if (b.data == ptr) {
                b.in_use = false;
                used_capacity_ -= b.size;
                break;
            }
        }
    });
    
    blocks_.push_back(std::move(new_block));
    total_capacity_ += new_block_size;
    used_capacity_ += size;
    
    return new_block.shared_ptr;
}

void TensorMemoryPool::deallocate(void* ptr) {
    // Memory is automatically deallocated through shared_ptr custom deleter
    (void)ptr; // Suppress unused parameter warning
}

void TensorMemoryPool::compact() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove completely free blocks (except the first one)
    blocks_.erase(
        std::remove_if(blocks_.begin() + 1, blocks_.end(),
                      [](const MemoryBlock& block) { return !block.in_use; }),
        blocks_.end()
    );
}

// EnhancedGGUFParser implementation
EnhancedGGUFParser::EnhancedGGUFParser(std::shared_ptr<TensorMemoryPool> memory_pool)
    : memory_pool_(memory_pool) {
    if (!memory_pool_) {
        memory_pool_ = std::make_shared<TensorMemoryPool>();
    }
}

std::optional<GGUFMetadata> EnhancedGGUFParser::parse_header(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open GGUF file: " << file_path << std::endl;
        return std::nullopt;
    }
    
    GGUFMetadata metadata;
    if (!read_header(file, metadata)) {
        return std::nullopt;
    }
    
    if (!read_kv_pairs(file, metadata)) {
        return std::nullopt;
    }
    
    return metadata;
}

std::optional<std::vector<GGUTensorInfo>> EnhancedGGUFParser::parse_tensor_info(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }
    
    GGUFMetadata metadata;
    if (!read_header(file, metadata)) {
        return std::nullopt;
    }
    
    // Skip KV pairs
    file.seekg(static_cast<std::streamoff>(metadata.kv_count * 8), std::ios::cur);
    
    std::vector<GGUTensorInfo> tensor_infos;
    if (!read_tensor_infos(file, metadata, tensor_infos)) {
        return std::nullopt;
    }
    
    return tensor_infos;
}

std::optional<std::vector<float>> EnhancedGGUFParser::load_tensor_float(const std::string& file_path,
                                                                       const GGUTensorInfo& tensor_info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check cache first
    auto cache_it = tensor_cache_.find(tensor_info.name);
    if (cache_it != tensor_cache_.end() && !cache_it->second.float_data.empty()) {
        update_cache_access(tensor_info.name);
        return cache_it->second.float_data;
    }
    
    // Load from file
    auto data = load_tensor_data<float>(file_path, tensor_info);
    if (!data) {
        return std::nullopt;
    }
    
    // Update cache
    CachedTensor& cached = tensor_cache_[tensor_info.name];
    cached.float_data = *data;
    cached.is_quantized = false;
    cached.access_count = 1;
    cached.last_access = std::chrono::steady_clock::now();
    
    evict_from_cache_if_needed();
    
    // Update memory stats
    memory_stats_.total_tensors_loaded++;
    memory_stats_.total_memory_used += data->size() * sizeof(float);
    
    return data;
}

std::optional<std::vector<int8_t>> EnhancedGGUFParser::load_tensor_quantized(const std::string& file_path,
                                                                            const GGUTensorInfo& tensor_info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check cache first
    auto cache_it = tensor_cache_.find(tensor_info.name);
    if (cache_it != tensor_cache_.end() && !cache_it->second.quantized_data.empty()) {
        update_cache_access(tensor_info.name);
        return cache_it->second.quantized_data;
    }
    
    // Load float data first if needed
    std::vector<float> float_data;
    if (tensor_info.type == GGUFValueType::T3_K_QUANTIZED || 
        tensor_info.type == GGUFValueType::BASE81_QUANTIZED) {
        // Load quantized data directly
        auto quantized_data = load_tensor_data<int8_t>(file_path, tensor_info);
        if (!quantized_data) {
            return std::nullopt;
        }
        
        // Update cache
        CachedTensor& cached = tensor_cache_[tensor_info.name];
        cached.quantized_data = *quantized_data;
        cached.is_quantized = true;
        cached.access_count = 1;
        cached.last_access = std::chrono::steady_clock::now();
        
        evict_from_cache_if_needed();
        
        return quantized_data;
    } else {
        // Load float data and quantize
        auto float_data_opt = load_tensor_float(file_path, tensor_info);
        if (!float_data_opt) {
            return std::nullopt;
        }
        
        auto quantized_data = quantize_to_ternary(*float_data_opt);
        
        // Update cache
        CachedTensor& cached = tensor_cache_[tensor_info.name];
        cached.quantized_data = quantized_data;
        cached.float_data = *float_data_opt;
        cached.is_quantized = true;
        cached.access_count = 1;
        cached.last_access = std::chrono::steady_clock::now();
        
        evict_from_cache_if_needed();
        
        // Update memory stats
        size_t original_size = float_data_opt->size() * sizeof(float);
        size_t quantized_size = quantized_data.size() * sizeof(int8_t);
        memory_stats_.quantized_memory_saved += (original_size - quantized_size);
        memory_stats_.compression_ratio = static_cast<float>(original_size) / quantized_size;
        
        return quantized_data;
    }
}

bool EnhancedGGUFParser::validate_tensor(const std::string& file_path, const GGUTensorInfo& tensor_info) {
    auto file_size = gguf_utils::get_file_size(file_path);
    if (file_size == 0) {
        return false;
    }
    
    // Check if tensor offset is valid
    if (tensor_info.offset >= file_size) {
        std::cerr << "Error: Tensor offset exceeds file size" << std::endl;
        return false;
    }
    
    // Check if tensor data fits in file
    if (tensor_info.offset + tensor_info.byte_size > file_size) {
        std::cerr << "Error: Tensor data exceeds file size" << std::endl;
        return false;
    }
    
    // Validate dimensions
    if (!gguf_utils::validate_tensor_dimensions(tensor_info.dimensions)) {
        return false;
    }
    
    return true;
}

EnhancedGGUFParser::MemoryStats EnhancedGGUFParser::get_memory_stats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return memory_stats_;
}

void EnhancedGGUFParser::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    tensor_cache_.clear();
    memory_stats_ = MemoryStats{};
}

// Private implementation methods
bool EnhancedGGUFParser::read_header(std::ifstream& file, GGUFMetadata& metadata) {
    // Read magic number
    file.read(reinterpret_cast<char*>(&metadata.magic), sizeof(metadata.magic));
    if (metadata.magic != GGUF_MAGIC) {
        std::cerr << "Error: Invalid GGUF magic number" << std::endl;
        return false;
    }
    
    // Read version
    file.read(reinterpret_cast<char*>(&metadata.version), sizeof(metadata.version));
    if (metadata.version > GGUF_VERSION) {
        std::cerr << "Error: Unsupported GGUF version: " << metadata.version << std::endl;
        return false;
    }
    
    // Read tensor and KV counts
    file.read(reinterpret_cast<char*>(&metadata.tensor_count), sizeof(metadata.tensor_count));
    file.read(reinterpret_cast<char*>(&metadata.kv_count), sizeof(metadata.kv_count));
    
    return true;
}

bool EnhancedGGUFParser::read_kv_pairs(std::ifstream& file, const GGUFMetadata& metadata) {
    // Skip KV pair reading for now - this would need full GGUF specification implementation
    file.seekg(static_cast<std::streamoff>(metadata.kv_count * 16), std::ios::cur);
    return true;
}

bool EnhancedGGUFParser::read_tensor_infos(std::ifstream& file, const GGUFMetadata& metadata,
                                          std::vector<GGUTensorInfo>& tensor_infos) {
    tensor_infos.reserve(metadata.tensor_count);
    
    for (uint32_t i = 0; i < metadata.tensor_count; ++i) {
        GGUTensorInfo tensor_info;
        
        // Read tensor name length and name
        uint32_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        
        std::vector<char> name_buf(name_len);
        file.read(name_buf.data(), name_len);
        tensor_info.name = std::string(name_buf.data(), name_len);
        
        // Read number of dimensions
        uint32_t n_dims;
        file.read(reinterpret_cast<char*>(&n_dims), sizeof(n_dims));
        
        // Read dimensions
        tensor_info.dimensions.resize(n_dims);
        for (uint32_t j = 0; j < n_dims; ++j) {
            file.read(reinterpret_cast<char*>(&tensor_info.dimensions[j]), sizeof(uint32_t));
        }
        
        // Calculate element count
        tensor_info.element_count = 1;
        for (uint32_t dim : tensor_info.dimensions) {
            tensor_info.element_count *= dim;
        }
        
        // Read tensor type
        uint32_t type_val;
        file.read(reinterpret_cast<char*>(&type_val), sizeof(type_val));
        tensor_info.type = static_cast<GGUFValueType>(type_val);
        
        // Read tensor offset
        file.read(reinterpret_cast<char*>(&tensor_info.offset), sizeof(tensor_info.offset));
        
        // Calculate byte size
        switch (tensor_info.type) {
            case GGUFValueType::FLOAT32:
                tensor_info.byte_size = tensor_info.element_count * sizeof(float);
                break;
            case GGUFValueType::INT8:
            case GGUFValueType::UINT8:
            case GGUFValueType::T3_K_QUANTIZED:
            case GGUFValueType::BASE81_QUANTIZED:
                tensor_info.byte_size = tensor_info.element_count * sizeof(int8_t);
                break;
            default:
                tensor_info.byte_size = tensor_info.element_count * 4; // Default to 4 bytes
                break;
        }
        
        // Check if quantized
        tensor_info.is_quantized = (tensor_info.type == GGUFValueType::T3_K_QUANTIZED ||
                                   tensor_info.type == GGUFValueType::BASE81_QUANTIZED);
        
        tensor_infos.push_back(std::move(tensor_info));
    }
    
    return true;
}

template<typename T>
std::optional<std::vector<T>> EnhancedGGUFParser::load_tensor_data(const std::string& file_path,
                                                                  const GGUTensorInfo& tensor_info) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }
    
    // Seek to tensor data
    file.seekg(static_cast<std::streamoff>(tensor_info.offset));
    
    std::vector<T> data(tensor_info.element_count);
    file.read(reinterpret_cast<char*>(data.data()), tensor_info.byte_size);
    
    if (file.gcount() != static_cast<std::streamsize>(tensor_info.byte_size)) {
        std::cerr << "Error: Failed to read complete tensor data" << std::endl;
        return std::nullopt;
    }
    
    return data;
}

std::vector<int8_t> EnhancedGGUFParser::quantize_to_ternary(const std::vector<float>& data) {
    T3KQuantizer quantizer;
    return quantizer.quantize(data);
}

std::vector<float> EnhancedGGUFParser::dequantize_from_ternary(const std::vector<int8_t>& data) {
    T3KQuantizer quantizer;
    return quantizer.dequantize(data);
}

void EnhancedGGUFParser::evict_from_cache_if_needed() {
    size_t total_cache_memory = 0;
    for (const auto& [name, tensor] : tensor_cache_) {
        total_cache_memory += tensor.float_data.size() * sizeof(float);
        total_cache_memory += tensor.quantized_data.size() * sizeof(int8_t);
    }
    
    // Evict by LRU if over memory limit
    while (total_cache_memory > MAX_CACHE_MEMORY || tensor_cache_.size() > MAX_CACHE_SIZE) {
        auto oldest_it = std::min_element(tensor_cache_.begin(), tensor_cache_.end(),
            [](const auto& a, const auto& b) {
                return a.second.last_access < b.second.last_access;
            });
        
        if (oldest_it != tensor_cache_.end()) {
            total_cache_memory -= oldest_it->second.float_data.size() * sizeof(float);
            total_cache_memory -= oldest_it->second.quantized_data.size() * sizeof(int8_t);
            tensor_cache_.erase(oldest_it);
        } else {
            break;
        }
    }
}

void EnhancedGGUFParser::update_cache_access(const std::string& tensor_name) {
    auto it = tensor_cache_.find(tensor_name);
    if (it != tensor_cache_.end()) {
        it->second.access_count++;
        it->second.last_access = std::chrono::steady_clock::now();
    }
}

// Utility functions implementation
namespace gguf_utils {

bool is_valid_gguf(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    return magic == GGUF_MAGIC;
}

size_t get_file_size(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 0;
    }
    return static_cast<size_t>(file.tellg());
}

size_t calculate_tensor_size(const GGUTensorInfo& tensor) {
    return tensor.byte_size;
}

bool validate_tensor_dimensions(const std::vector<uint32_t>& dims) {
    if (dims.empty()) {
        return false;
    }
    
    for (uint32_t dim : dims) {
        if (dim == 0) {
            return false;
        }
        if (dim > 1000000) { // Reasonable upper limit
            std::cerr << "Warning: Very large tensor dimension: " << dim << std::endl;
        }
    }
    
    return true;
}

std::string type_to_string(GGUFValueType type) {
    switch (type) {
        case GGUFValueType::FLOAT32: return "FLOAT32";
        case GGUFValueType::INT8: return "INT8";
        case GGUFValueType::UINT8: return "UINT8";
        case GGUFValueType::T3_K_QUANTIZED: return "T3_K_QUANTIZED";
        case GGUFValueType::BASE81_QUANTIZED: return "BASE81_QUANTIZED";
        default: return "UNKNOWN";
    }
}

float get_quantization_ratio(GGUFValueType original_type, GGUFValueType quantized_type) {
    size_t original_size = 4; // Assume float32
    size_t quantized_size = 1; // Assume int8
    
    if (original_type == GGUFValueType::FLOAT32) original_size = 4;
    else if (original_type == GGUFValueType::FLOAT64) original_size = 8;
    
    if (quantized_type == GGUFValueType::T3_K_QUANTIZED) quantized_size = 1;
    else if (quantized_type == GGUFValueType::BASE81_QUANTIZED) quantized_size = 1;
    
    return static_cast<float>(original_size) / quantized_size;
}

} // namespace gguf_utils

// Explicit template instantiations
template std::optional<std::vector<float>> EnhancedGGUFParser::load_tensor_data<float>(
    const std::string& file_path, const GGUTensorInfo& tensor_info);
template std::optional<std::vector<int8_t>> EnhancedGGUFParser::load_tensor_data<int8_t>(
    const std::string& file_path, const GGUTensorInfo& tensor_info);

} // namespace t81::codec
