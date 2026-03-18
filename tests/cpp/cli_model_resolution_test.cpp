#include "test_runtime_check.hpp"
#include "t81/cli/driver.hpp"
#include "t81/weights.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path make_temp_path(const std::string& prefix) {
  static std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<uint64_t> dist;
  return fs::temp_directory_path() / (prefix + "-" + std::to_string(dist(rng)));
}

void write_file(const fs::path& path, std::string_view content = "stub") {
  std::ofstream out(path, std::ios::binary);
  T81_TEST_CHECK(static_cast<bool>(out));
  out << content;
}

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

void write_minimal_gguf(const fs::path& path, uint32_t tensor_type) {
  std::ofstream out(path, std::ios::binary);
  T81_TEST_CHECK(static_cast<bool>(out));

  out.write("GGUF", 4);
  write_le<uint32_t>(out, 3);
  write_le<uint64_t>(out, 1);
  write_le<uint64_t>(out, 0);

  write_gguf_string(out, "weight");
  write_le<uint32_t>(out, 1);
  write_le<uint64_t>(out, 16);
  write_le<uint32_t>(out, tensor_type);
  write_le<uint64_t>(out, 0);
}

}  // namespace

int main() {
  const fs::path original_cwd = fs::current_path();
  const fs::path root = make_temp_path("t81-cli-model-resolution");
  std::error_code ec;
  fs::create_directories(root / "models", ec);
  T81_TEST_CHECK(!ec);
  fs::create_directories(root / "model", ec);
  T81_TEST_CHECK(!ec);

  write_file(root / "models" / "tinyllama-1.1b.Q2_K.gguf");
  write_file(root / "model" / "native-demo.t81w");

  fs::current_path(root);

  {
    std::string error;
    auto resolved = t81::cli::resolve_repo_model_path("tinyllama-1.1b.Q2_K", {".gguf"}, &error);
    T81_TEST_CHECK(resolved.has_value());
    T81_TEST_CHECK(resolved->filename() == "tinyllama-1.1b.Q2_K.gguf");
  }

  {
    std::string error;
    auto resolved =
        t81::cli::resolve_repo_model_path("native-demo", {".t81w"}, &error);
    T81_TEST_CHECK(resolved.has_value());
    T81_TEST_CHECK(resolved->filename() == "native-demo.t81w");
  }

  {
    write_file(root / "models" / "dupe.gguf");
    write_file(root / "model" / "dupe.gguf");
    std::string error;
    auto resolved = t81::cli::resolve_repo_model_path("dupe", {".gguf"}, &error);
    T81_TEST_CHECK(!resolved.has_value());
    T81_TEST_CHECK(error.find("ambiguous") != std::string::npos);
  }

  {
    const fs::path import_root = root / "import-case";
    fs::create_directories(import_root / "models", ec);
    T81_TEST_CHECK(!ec);

    t81::weights::NativeModel model;
    t81::weights::NativeTensor tensor;
    tensor.shape = {2, 2};
    tensor.trits = 4;
    tensor.data = {40};
    model["mat_a"] = tensor;

    const fs::path source = import_root / "models" / "native-import-demo.t81w";
    t81::weights::save_t81w(model, source);

    fs::current_path(import_root);
    std::string error;
    auto resolved = t81::cli::resolve_repo_model_path("native-import-demo", {".t81w"}, &error);
    T81_TEST_CHECK(resolved.has_value());
    auto loaded = t81::cli::load_weights_model("native-import-demo", &error);
    T81_TEST_CHECK(static_cast<bool>(loaded));
    T81_TEST_CHECK(loaded->native.find("mat_a") != loaded->native.end());
    fs::current_path(root);
  }

  {
    const fs::path unsupported = root / "models" / "unsupported.gguf";
    write_minimal_gguf(unsupported, 12);
    bool threw = false;
    try {
      static_cast<void>(t81::weights::load_gguf(unsupported));
    } catch (const std::runtime_error& ex) {
      threw = true;
      const std::string msg = ex.what();
      T81_TEST_CHECK(msg.find("does not contain any T3_K tensors") != std::string::npos);
      T81_TEST_CHECK(msg.find("Q4_K=1") != std::string::npos);
    }
    T81_TEST_CHECK(threw);
  }

  fs::current_path(original_cwd);
  fs::remove_all(root, ec);
  std::cout << "cli model resolution test passed!\n";
  return 0;
}
