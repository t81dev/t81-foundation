#include <iostream>
#include <fstream>
#include <vector>
#include <random>

int main() {
    std::cout << "Creating a 3B-sized test model for 8GB RAM...\n";
    
    // Create a GGUF file that's ~2GB (3B model size)
    std::ofstream model_file("/Users/t81dev/Code/t81-foundation/models/test_3b_model.gguf", std::ios::binary);
    
    if (!model_file.is_open()) {
        std::cerr << "Failed to create model file\n";
        return 1;
    }
    
    // GGUF magic number
    const char* magic = "GGUF";
    model_file.write(magic, 4);
    
    // Version (little endian)
    uint32_t version = 3;
    model_file.write(reinterpret_cast<const char*>(&version), 4);
    
    // Tensor count (more tensors for larger model)
    uint64_t tensor_count = 100;
    model_file.write(reinterpret_cast<const char*>(&tensor_count), 8);
    
    // KV count (more metadata for larger model)
    uint64_t kv_count = 20;
    model_file.write(reinterpret_cast<const char*>(&kv_count), 8);
    
    // Write KV pairs for a 3B model
    std::vector<std::pair<std::string, std::string>> kv_pairs = {
        {"general.architecture", "llama"},
        {"llama.vocab_size", "32000"},
        {"llama.context_length", "4096"},
        {"llama.embedding_length", "3072"},
        {"llama.feed_forward_length", "8192"},
        {"llama.attention.head_count", "32"},
        {"llama.attention.head_count_kv", "4"},
        {"llama.block_count", "32"},
        {"llama.rope.dimension_count", "128"},
        {"llama.rope.freq_base", "10000.0"},
        {"llama.attention.layer_norm_rms_epsilon", "0.000001"},
        {"llama.attention.causal", "true"},
        {"llama.pooling_type", "none"},
        {"llama.logit_scale", "0.0"},
        {"llama.parallel_residual", "true"},
        {"llama.attention.q_lora_rank", "0"},
        {"llama.attention.kv_lora_rank", "0"},
        {"llama.attention.q_lora_alpha", "0"},
        {"llama.attention.kv_lora_alpha", "0"}
    };
    
    for (const auto& [key, value] : kv_pairs) {
        // Key length and key
        uint32_t key_len = key.length();
        model_file.write(reinterpret_cast<const char*>(&key_len), 4);
        model_file.write(key.c_str(), key_len);
        
        // Value type (8 = string)
        uint32_t value_type = 8;
        model_file.write(reinterpret_cast<const char*>(&value_type), 4);
        
        // Value length and value
        uint32_t value_len = value.length();
        model_file.write(reinterpret_cast<const char*>(&value_len), 4);
        model_file.write(value.c_str(), value_len);
    }
    
    // Write tensor info for 100 tensors
    std::vector<std::string> tensor_names = {
        "token_embd.weight", "output_norm.weight", "output.weight",
        "blk.0.attn_q.weight", "blk.0.attn_k.weight", "blk.0.attn_v.weight", "blk.0.attn_o.weight",
        "blk.0.ffn_gate.weight", "blk.0.ffn_down.weight", "blk.0.ffn_up.weight",
        "blk.0.attn_norm.weight", "blk.0.ffn_norm.weight"
    };
    
    // Generate tensor names for all blocks
    for (int block = 0; block < 32; ++block) {
        for (const auto& base_name : {"attn_q.weight", "attn_k.weight", "attn_v.weight", "attn_o.weight", 
                                      "ffn_gate.weight", "ffn_down.weight", "ffn_up.weight", 
                                      "attn_norm.weight", "ffn_norm.weight"}) {
            std::string name = "blk." + std::to_string(block) + "." + base_name;
            tensor_names.push_back(name);
        }
    }
    
    // Write tensor metadata
    for (size_t i = 0; i < tensor_names.size() && i < 100; ++i) {
        const auto& tensor_name = tensor_names[i];
        
        // Tensor name
        uint32_t name_len = tensor_name.length();
        model_file.write(reinterpret_cast<const char*>(&name_len), 4);
        model_file.write(tensor_name.c_str(), name_len);
        
        // Tensor dimensions (various sizes for different layers)
        uint32_t n_dims = 2;
        model_file.write(reinterpret_cast<const char*>(&n_dims), 4);
        
        uint64_t dim1 = 3072;  // embedding dimension
        uint64_t dim2 = (tensor_name.find("token_embd") != std::string::npos) ? 32000 : 3072;
        if (tensor_name.find("attn_q") != std::string::npos) dim2 = 3072;
        else if (tensor_name.find("attn_k") != std::string::npos) dim2 = 384;
        else if (tensor_name.find("attn_v") != std::string::npos) dim2 = 384;
        else if (tensor_name.find("attn_o") != std::string::npos) dim2 = 3072;
        else if (tensor_name.find("ffn_gate") != std::string::npos) dim2 = 8192;
        else if (tensor_name.find("ffn_down") != std::string::npos) dim2 = 8192;
        else if (tensor_name.find("ffn_up") != std::string::npos) dim2 = 8192;
        
        model_file.write(reinterpret_cast<const char*>(&dim1), 8);
        model_file.write(reinterpret_cast<const char*>(&dim2), 8);
        
        // Tensor type (0 = F32)
        uint32_t tensor_type = 0;
        model_file.write(reinterpret_cast<const char*>(&tensor_type), 4);
        
        // Tensor offset (will be after header)
        uint64_t offset = 0;
        model_file.write(reinterpret_cast<const char*>(&offset), 8);
    }
    
    // Write tensor data (make it ~2GB total)
    std::cout << "Generating ~2GB of tensor data...\n";
    const size_t target_size = 2UL * 1024 * 1024 * 1024; // 2GB
    const size_t chunk_size = 1024 * 1024; // 1MB chunks
    
    std::vector<float> chunk_data(chunk_size / sizeof(float));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 0.1f);
    
    for (auto& val : chunk_data) {
        val = dist(gen);
    }
    
    size_t written = 0;
    while (written < target_size) {
        size_t to_write = std::min(chunk_size, target_size - written);
        model_file.write(reinterpret_cast<const char*>(chunk_data.data()), to_write);
        written += to_write;
        
        // Progress indicator
        if (written % (100 * 1024 * 1024) == 0) {
            std::cout << "Written " << (written / (1024 * 1024)) << " MB...\n";
        }
    }
    
    model_file.close();
    
    std::cout << "✅ 3B-sized test model created: test_3b_model.gguf\n";
    std::cout << "📊 Size: " << (target_size / 1024.0 / 1024.0) << " MB\n";
    std::cout << "🧠 This should fit well in 8GB RAM with room to spare\n";
    
    return 0;
}
