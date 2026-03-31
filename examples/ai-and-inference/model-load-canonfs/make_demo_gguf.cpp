#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

template <typename T>
void write_le(std::ofstream& out, T value) {
  using U = std::make_unsigned_t<T>;
  U bits = static_cast<U>(value);
  for (size_t i = 0; i < sizeof(U); ++i) {
    out.put(static_cast<char>((bits >> (8 * i)) & 0xFFu));
  }
}

void write_gguf_string(std::ofstream& out, std::string_view value) {
  write_le<uint64_t>(out, static_cast<uint64_t>(value.size()));
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void write_gguf_u32_kv(std::ofstream& out, std::string_view key, uint32_t value) {
  write_gguf_string(out, key);
  write_le<uint32_t>(out, 4);
  write_le<uint32_t>(out, value);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: t81_make_demo_gguf <out.gguf>\n";
    return 2;
  }

  const std::filesystem::path output = argv[1];
  std::ofstream out(output, std::ios::binary);
  if (!out) {
    std::cerr << "unable to create " << output.string() << "\n";
    return 1;
  }

  out.write("GGUF", 4);
  write_le<uint32_t>(out, 3);
  write_le<uint64_t>(out, 1);
  write_le<uint64_t>(out, 4);

  write_gguf_string(out, "general.architecture");
  write_le<uint32_t>(out, 8);
  write_gguf_string(out, "llama");
  write_gguf_u32_kv(out, "llama.block_count", 32);
  write_gguf_u32_kv(out, "llama.embedding_length", 4096);
  write_gguf_u32_kv(out, "llama.attention.head_count", 32);

  write_gguf_string(out, "model.layers.0.self_attn.q_proj.weight");
  write_le<uint32_t>(out, 2);
  write_le<uint64_t>(out, 16);
  write_le<uint64_t>(out, 8);
  write_le<uint32_t>(out, 0);
  write_le<uint64_t>(out, 0);

  const auto meta_size = static_cast<size_t>(out.tellp());
  const size_t aligned = (meta_size + 31u) & ~size_t(31u);
  for (size_t i = meta_size; i < aligned; ++i) {
    out.put('\0');
  }

  for (uint64_t i = 0; i < 16 * 8; ++i) {
    const float value = static_cast<float>(i) * 0.25f;
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }

  out.close();
  std::cout << "wrote=" << output.string() << "\n";
  std::cout << "arch=llama\n";
  return 0;
}
