#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/weights.hpp"

namespace fs = std::filesystem;

namespace t81::cli {

using namespace t81::canonfs;

std::vector<std::byte> serialize_tensor(const t81::weights::NativeTensor& tensor) {
  // Format per RFC:
  // 1. Type ID (1 byte): 0x20
  // 2. Header (71 bytes):
  //    version (1 byte): 1
  //    format (1 byte): NativeFormat enum value
  //    rank (1 byte)
  //    reserved (4 bytes): 0
  //    shape (64 bytes): 8 uint64_t little-endian
  // 3. Data Payload

  std::vector<std::byte> buffer;
  // Payload size: tensor.data.size() * 8 bytes (assuming uint64_t elements)
  size_t estimated_size = 1 + 72 + tensor.data.size() * 8;
  buffer.reserve(estimated_size);

  // 1. Type ID
  buffer.push_back(static_cast<std::byte>(0x20));

  // 2. Header
  // version
  buffer.push_back(static_cast<std::byte>(1));

  // format
  buffer.push_back(static_cast<std::byte>(static_cast<uint8_t>(tensor.format)));

  // rank
  uint8_t rank = static_cast<uint8_t>(tensor.shape.size());
  buffer.push_back(static_cast<std::byte>(rank));

  // reserved (4 bytes)
  for (int i = 0; i < 4; ++i) {
    buffer.push_back(static_cast<std::byte>(0));
  }

  // shape (64 bytes): 8 uint64_t little-endian
  for (int i = 0; i < 8; ++i) {
    uint64_t dim = (i < rank) ? tensor.shape[i] : 0;
    for (int b = 0; b < 8; ++b) {
      buffer.push_back(static_cast<std::byte>((dim >> (b * 8)) & 0xFF));
    }
  }

  // 3. Data Payload
  // tensor.data is vector<uint64_t>.
  for (uint64_t val : tensor.data) {
    for (int b = 0; b < 8; ++b) {
      buffer.push_back(static_cast<std::byte>((val >> (b * 8)) & 0xFF));
    }
  }

  return buffer;
}

int canonize_tensor(const std::string& input_file, const fs::path& canonfs_root) {
  fs::path input_path(input_file);
  if (!fs::exists(input_path)) {
    std::cerr << "Input file not found: " << input_file << "\n";
    return 1;
  }

  t81::weights::ModelFile model;
  try {
    std::string ext = input_path.extension().string();
    if (ext == ".t81w") {
      model = t81::weights::load_t81w(input_path);
    } else if (ext == ".safetensors") {
      model = t81::weights::load_safetensors(input_path);
    } else {
      std::cerr << "Unsupported extension: " << ext << "\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error loading model: " << e.what() << "\n";
    return 1;
  }

  std::error_code ec;
  fs::create_directories(canonfs_root, ec);
  if (ec) {
    std::cerr << "Failed to create directory " << canonfs_root << ": " << ec.message() << "\n";
    return 1;
  }
  auto driver = t81::canonfs::make_persistent_driver(canonfs_root);

  std::cout << "Canonizing tensors from " << input_file << " into " << canonfs_root.string()
            << "...\n";

  for (const auto& [name, tensor] : model.native) {
    auto bytes = serialize_tensor(tensor);
    auto result = driver->write_object(t81::canonfs::ObjectType::CanonTensor, bytes);
    if (!result) {
      std::cerr << "Failed to write tensor " << name << "\n";
      continue;
    }

    auto ref = result.value();
    std::cout << name << ": sha3-256:" << ref.hash.h.to_string() << "\n";
  }

  return 0;
}

}  // namespace t81::cli
