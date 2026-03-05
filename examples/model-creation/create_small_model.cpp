#include <fstream>
#include <iostream>
#include <random>
#include <vector>

int main() {
  std::cout << "Creating a small test model for 8GB RAM...\n";

  // Create a minimal GGUF file that llama.cpp can load
  std::ofstream model_file("/Users/t81dev/Code/t81-foundation/models/small_test_model.gguf",
                           std::ios::binary);

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

  // Tensor count (1 small tensor)
  uint64_t tensor_count = 1;
  model_file.write(reinterpret_cast<const char*>(&tensor_count), 8);

  // KV count (minimal metadata)
  uint64_t kv_count = 5;
  model_file.write(reinterpret_cast<const char*>(&kv_count), 8);

  // Write minimal KV pairs
  std::vector<std::pair<std::string, std::string>> kv_pairs = {
      {"general.architecture", "llama"},
      {"llama.vocab_size", "1000"},
      {"llama.context_length", "512"},
      {"llama.embedding_length", "128"},
      {"llama.feed_forward_length", "512"}};

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

  // Write tensor info
  std::string tensor_name = "token_embd.weight";
  uint32_t name_len = tensor_name.length();
  model_file.write(reinterpret_cast<const char*>(&name_len), 4);
  model_file.write(tensor_name.c_str(), name_len);

  // Tensor dimensions (128 x 1000)
  uint32_t n_dims = 2;
  model_file.write(reinterpret_cast<const char*>(&n_dims), 4);

  uint64_t dim1 = 128;
  uint64_t dim2 = 1000;
  model_file.write(reinterpret_cast<const char*>(&dim1), 8);
  model_file.write(reinterpret_cast<const char*>(&dim2), 8);

  // Tensor type (0 = F32)
  uint32_t tensor_type = 0;
  model_file.write(reinterpret_cast<const char*>(&tensor_type), 4);

  // Tensor offset (will be after header)
  uint64_t offset = 0;
  model_file.write(reinterpret_cast<const char*>(&offset), 8);

  // Write some dummy tensor data (128 * 1000 floats = 512KB)
  std::vector<float> tensor_data(128 * 1000);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<float> dist(0.0f, 0.1f);

  for (auto& val : tensor_data) {
    val = dist(gen);
  }

  model_file.write(reinterpret_cast<const char*>(tensor_data.data()),
                   tensor_data.size() * sizeof(float));

  model_file.close();

  std::cout << "✅ Small test model created: small_test_model.gguf\n";
  std::cout << "📊 Size: " << (tensor_data.size() * sizeof(float) / 1024.0) << " KB\n";
  std::cout << "🧠 This should easily fit in 8GB RAM\n";

  return 0;
}
