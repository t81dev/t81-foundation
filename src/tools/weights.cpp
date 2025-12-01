#include "t81/weights.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace t81::weights {

namespace {
// Packs balanced trits (-1,0,+1) into 48-trit limbs.
NativeTensor pack_trits(std::span<const int8_t> src,
                        const std::vector<uint64_t>& shape) {
    NativeTensor tensor;
    tensor.shape = shape;
    tensor.data.clear();
    tensor.data.reserve((src.size() + 47) / 48);

    for (size_t offset = 0; offset < src.size(); offset += 48) {
        uint64_t limb = 0;
        size_t count = std::min<size_t>(48, src.size() - offset);
        for (size_t i = 0; i < count; ++i) {
            int8_t trit = src[offset + i];
            if (trit < -1 || trit > 1) {
                throw std::runtime_error("invalid trit value");
            }
            limb = limb * 3 + static_cast<uint64_t>(trit + 1);
        }
        for (size_t i = count; i < 48; ++i) {
            limb *= 3;
        }
        tensor.data.push_back(limb);
    }
    return tensor;
}

std::string format(uint64_t bytes) {
    constexpr const char* const units[] = {"B", "KB", "MB", "GB"};
    double value = static_cast<double>(bytes);
    size_t idx = 0;
    while (idx < 3 && value >= 1024.0) {
        value /= 1024.0;
        ++idx;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f %s", value, units[idx]);
    return buf;
}
} // namespace

ModelFile load_gguf(const std::filesystem::path&) {
    throw std::runtime_error("GGUF loader is not implemented yet");
}

ModelFile load_safetensors(const std::filesystem::path&) {
    ModelFile mf;
    throw std::runtime_error("SafeTensors loader is not implemented yet");
    return mf;
}

NativeTensor import_bitnet_b158(std::span<const int8_t> src,
                                const std::vector<uint64_t>& shape) {
    return pack_trits(src, shape);
}

void save_t81w(const NativeModel& model,
               const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot write " + path.string());
    }
    out << "T81W1\n";
    uint64_t num_tensors = model.size();
    out.write(reinterpret_cast<const char*>(&num_tensors), sizeof(num_tensors));
    for (const auto& [name, tensor] : model) {
        uint64_t name_len = name.size();
        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name.data(), name_len);
        uint64_t rank = tensor.shape.size();
        out.write(reinterpret_cast<const char*>(&rank), sizeof(rank));
        for (auto d : tensor.shape) {
            out.write(reinterpret_cast<const char*>(&d), sizeof(d));
        }
        uint64_t trits = tensor.num_trits();
        out.write(reinterpret_cast<const char*>(&trits), sizeof(trits));
        for (uint64_t limb : tensor.data) {
            out.write(reinterpret_cast<const char*>(&limb), sizeof(limb));
        }
    }
}

NativeModel load_t81w(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path.string());
    char magic[6] = {};
    in.read(magic, 5);
    if (std::string_view(magic, 5) != "T81W1") {
        throw std::runtime_error("invalid t81w file");
    }
    uint64_t num_tensors = 0;
    in.read(reinterpret_cast<char*>(&num_tensors), sizeof(num_tensors));
    NativeModel model;
    for (uint64_t i = 0; i < num_tensors; ++i) {
        uint64_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        std::string name(name_len, '\0');
        in.read(name.data(), name_len);
        uint64_t rank = 0;
        in.read(reinterpret_cast<char*>(&rank), sizeof(rank));
        std::vector<uint64_t> shape(rank);
        for (uint64_t& d : shape) {
            in.read(reinterpret_cast<char*>(&d), sizeof(d));
        }
        uint64_t trits = 0;
        in.read(reinterpret_cast<char*>(&trits), sizeof(trits));
        auto& tensor = model[name];
        tensor.shape = shape;
        uint64_t limb_count = (trits + 47) / 48;
        tensor.data.resize(limb_count);
        for (uint64_t j = 0; j < limb_count; ++j) {
            in.read(reinterpret_cast<char*>(&tensor.data[j]), sizeof(uint64_t));
        }
    }
    return model;
}

void print_info(const ModelFile& mf) {
    std::cout << "Model contains " << mf.tensors.size() << " tensors, "
              << format(mf.total_trits / 5) << " avg density\n";
}

std::string format_bytes(uint64_t bytes) {
    return format(bytes);
}

} // namespace t81::weights
