#include "t81/crypto/sha3.hpp"
#include "t81/weights.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <variant>
#include <cctype>

namespace t81::weights {

namespace {
// Packs balanced trits (-1,0,+1) into 48-trit limbs.
NativeTensor pack_trits(std::span<const int8_t> src,
                        const std::vector<uint64_t>& shape) {
    NativeTensor tensor;
    tensor.shape = shape;
    tensor.data.clear();
    tensor.data.reserve((src.size() + 47) / 48);
    tensor.trits = src.size();

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

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : text_(text), idx_(0) {}

    JsonValue parse() {
        skip();
        return parse_value();
    }

private:
    const std::string& text_;
    size_t idx_;

    void skip() {
        while (idx_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[idx_]))) {
            ++idx_;
        }
    }

    char peek() const {
        return idx_ < text_.size() ? text_[idx_] : '\0';
    }

    char consume() {
        return idx_ < text_.size() ? text_[idx_++] : '\0';
    }

    JsonValue parse_value() {
        skip();
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string();
        if ((c >= '0' && c <= '9') || c == '-') return parse_number();
        throw std::runtime_error("JSON parse error");
    }

    JsonValue parse_object() {
        consume();
        std::map<std::string, JsonValue> map;
        skip();
        while (peek() != '}' && idx_ < text_.size()) {
            auto key = parse_string().string_value;
            skip();
            if (consume() != ':') throw std::runtime_error("JSON object missing ':'");
            auto value = parse_value();
            map.emplace(key, std::move(value));
            skip();
            if (peek() == ',') {
                consume();
                skip();
            } else {
                break;
            }
        }
        if (consume() != '}') throw std::runtime_error("JSON object missing '}'");
        return JsonValue::make_object(std::move(map));
    }

    JsonValue parse_array() {
        consume();
        std::vector<JsonValue> arr;
        skip();
        while (peek() != ']' && idx_ < text_.size()) {
            arr.push_back(parse_value());
            skip();
            if (peek() == ',') {
                consume();
                skip();
            } else {
                break;
            }
        }
        if (consume() != ']') throw std::runtime_error("JSON array missing ']'");
        return JsonValue::make_array(std::move(arr));
    }

    JsonValue parse_string() {
        consume();
        std::string out;
        while (idx_ < text_.size()) {
            char c = consume();
            if (c == '"') break;
            if (c == '\\') {
                char esc = consume();
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    default: out.push_back(esc); break;
                }
            } else {
                out.push_back(c);
            }
        }
        return JsonValue::make_string(std::move(out));
    }

    JsonValue parse_number() {
        size_t start = idx_;
        if (peek() == '-') ++idx_;
        while (std::isdigit(peek())) ++idx_;
        if (peek() == '.') {
            ++idx_;
            while (std::isdigit(peek())) ++idx_;
        }
        double value = std::stod(text_.substr(start, idx_ - start));
        return JsonValue::make_number(value);
    }
};

uint64_t json_to_uint(const JsonValue& val) {
    if (!val.is_number) throw std::runtime_error("JSON: expected number");
    return static_cast<uint64_t>(val.number_value);
}

std::vector<uint64_t> json_to_shape(const JsonValue& val) {
    if (val.array_value.empty()) throw std::runtime_error("JSON: expected array for shape");
    std::vector<uint64_t> shape;
    shape.reserve(val.array_value.size());
    for (const auto& entry : val.array_value) {
        shape.push_back(json_to_uint(entry));
    }
    return shape;
}

uint64_t product_of(const std::vector<uint64_t>& shape) {
    uint64_t acc = 1;
    for (uint64_t dim : shape) {
        if (dim == 0) throw std::runtime_error("shape dimension zero");
        if (acc > std::numeric_limits<uint64_t>::max() / dim) {
            throw std::overflow_error("shape overflow");
        }
        acc *= dim;
    }
    return acc;
}

uint64_t count_zero_trits(const NativeTensor& tensor) {
    uint64_t zeros = 0;
    uint64_t remaining = tensor.num_trits();
    for (uint64_t limb : tensor.data) {
        uint64_t value = limb;
        uint64_t digits = std::min<uint64_t>(48, remaining);
        for (uint64_t i = 0; i < digits; ++i) {
            if ((value % 3) == 1) {
                ++zeros;
            }
            value /= 3;
        }
        if (remaining <= 48) {
            break;
        }
        remaining -= digits;
    }
    return zeros;
}
} // namespace

ModelFile build_from_header(const JsonValue& root, const std::vector<uint8_t>& buffer) {
    ModelFile mf;
    for (const auto& [key, value] : root.object_value) {
        if (key.rfind("__", 0) == 0) continue;
        if (value.object_value.empty()) continue;
        auto dtype_it = value.object_value.find("dtype");
        if (dtype_it == value.object_value.end()) continue;
        const auto& dtype = dtype_it->second;
        if (!dtype.is_string || dtype.string_value != "I8") continue;
        auto shape_it = value.object_value.find("shape");
        if (shape_it == value.object_value.end()) continue;
        auto shape = json_to_shape(shape_it->second);
        uint64_t count = product_of(shape);

        auto offsets_it = value.object_value.find("data_offsets");
        auto lengths_it = value.object_value.find("data_lengths");
        if (offsets_it == value.object_value.end() || lengths_it == value.object_value.end()) continue;
        if (offsets_it->second.array_value.empty() || lengths_it->second.array_value.empty()) continue;
        uint64_t offset = json_to_uint(offsets_it->second.array_value[0]);
        uint64_t length = json_to_uint(lengths_it->second.array_value[0]);
        if (offset + length > buffer.size()) throw std::runtime_error("tensor data out of bounds");

        std::span<const int8_t> raw(reinterpret_cast<const int8_t*>(buffer.data() + offset), static_cast<size_t>(length));
        auto native = import_bitnet_b158(raw, shape);

        TensorInfo info;
        info.name = key;
        info.shape = shape;
        info.num_trits = count;
        mf.tensors.push_back(info);
        mf.total_trits += count;
        mf.total_parameters += count;
        mf.native.emplace(key, std::move(native));
    }
    return mf;
}

ModelFile load_gguf(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open GGUF file");
    uint32_t header_len = 0;
    f.read(reinterpret_cast<char*>(&header_len), sizeof(header_len));
    std::string header(header_len, '\0');
    f.read(header.data(), header_len);
    JsonParser parser(header);
    JsonValue root = parser.parse();
    auto file_size = std::filesystem::file_size(path);
    std::vector<uint8_t> buffer(file_size);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buffer.data()), file_size);
    auto mf = build_from_header(root, buffer);
    mf.format = "GGUF";
    return mf;
}

ModelFile load_safetensors(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open SafeTensors file");
    uint64_t header_len = 0;
    f.read(reinterpret_cast<char*>(&header_len), sizeof(header_len));
    std::string header(header_len, '\0');
    f.read(header.data(), header_len);
    JsonParser parser(header);
    JsonValue root = parser.parse();
    if (root.object_value.empty()) {
        throw std::runtime_error("SafeTensors: empty header");
    }
    ModelFile mf;
    auto file_size = std::filesystem::file_size(path);
    std::vector<uint8_t> buffer(file_size);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buffer.data()), file_size);

    for (const auto& [key, value] : root.object_value) {
        if (key.rfind("__", 0) == 0) continue;  // skip metadata
        if (value.object_value.empty()) continue;
        auto dtype_it = value.object_value.find("dtype");
        if (dtype_it == value.object_value.end()) continue;
        const auto& dtype = dtype_it->second;
        if (!dtype.is_string || dtype.string_value != "I8") continue;
        auto shape_it = value.object_value.find("shape");
        if (shape_it == value.object_value.end()) continue;
        auto shape = json_to_shape(shape_it->second);
        uint64_t count = product_of(shape);

        auto offsets_it = value.object_value.find("data_offsets");
        auto lengths_it = value.object_value.find("data_lengths");
        if (offsets_it == value.object_value.end() || lengths_it == value.object_value.end()) {
            throw std::runtime_error("SafeTensors: missing offsets/lengths");
        }
        if (offsets_it->second.array_value.empty() || lengths_it->second.array_value.empty()) {
            throw std::runtime_error("SafeTensors: empty offset/length arrays");
        }
        uint64_t offset = json_to_uint(offsets_it->second.array_value[0]);
        uint64_t length = json_to_uint(lengths_it->second.array_value[0]);
        if (offset + length > buffer.size()) {
            throw std::runtime_error("SafeTensors: data range out of bounds");
        }

        std::span<const int8_t> raw(reinterpret_cast<const int8_t*>(buffer.data() + offset), static_cast<size_t>(length));
        auto native = import_bitnet_b158(raw, shape);

        TensorInfo info;
        info.name = key;
        info.shape = shape;
        info.num_trits = count;
        mf.tensors.push_back(info);
        mf.total_trits += count;
        mf.total_parameters += count;
        mf.native.emplace(key, std::move(native));
    }
    mf.format = "SafeTensors";
    return mf;
}

NativeTensor import_bitnet_b158(std::span<const int8_t> src,
                                const std::vector<uint64_t>& shape) {
    return pack_trits(src, shape);
}

void save_t81w(const NativeModel& model,
               const std::filesystem::path& path) {
    std::vector<uint8_t> buffer;
    auto append_bytes = [&](const void* data, size_t size) {
        const auto* ptr = static_cast<const uint8_t*>(data);
        buffer.insert(buffer.end(), ptr, ptr + size);
    };
    auto append_le64 = [&](uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            buffer.push_back(static_cast<uint8_t>(value & 0xFF));
            value >>= 8;
        }
    };

    const std::string magic = "T81W1\n";
    append_bytes(magic.data(), magic.size());
    const size_t hash_pos = buffer.size();
    buffer.insert(buffer.end(), 128, '0');
    buffer.push_back('\n');
    const size_t payload_start = buffer.size();

    append_le64(model.size());
    for (const auto& [name, tensor] : model) {
        append_le64(name.size());
        append_bytes(name.data(), name.size());
        append_le64(tensor.shape.size());
        for (uint64_t dim : tensor.shape) {
            append_le64(dim);
        }
        const uint64_t trits = tensor.num_trits();
        append_le64(trits);
        const uint64_t limbs = (trits + 47) / 48;
        for (uint64_t li = 0; li < limbs; ++li) {
            uint64_t limb = li < tensor.data.size() ? tensor.data[li] : 0;
            append_le64(limb);
        }
    }

    auto payload = std::span<const uint8_t>(buffer.data() + payload_start,
                                            buffer.size() - payload_start);
    const std::string hash_hex = crypto::sha3_512_hex(payload);
    std::copy(hash_hex.begin(), hash_hex.end(), buffer.begin() + hash_pos);

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot write " + path.string());
    }
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

ModelFile load_t81w(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path.string());

    std::string magic;
    if (!std::getline(in, magic) || magic != "T81W1") {
        throw std::runtime_error("invalid t81w file");
    }
    std::string checksum;
    if (!std::getline(in, checksum) || checksum.size() != 128) {
        throw std::runtime_error("t81w: missing or malformed checksum");
    }

    uint64_t header_end = static_cast<uint64_t>(in.tellg());
    const uint64_t file_size = std::filesystem::file_size(path);
    if (file_size < header_end) {
        throw std::runtime_error("t81w: file truncated");
    }

    std::vector<uint8_t> payload(static_cast<size_t>(file_size - header_end));
    in.read(reinterpret_cast<char*>(payload.data()), payload.size());

    const std::string computed = crypto::sha3_512_hex(payload);
    if (computed != checksum) {
        throw std::runtime_error("t81w: checksum mismatch");
    }

    ModelFile mf;
    mf.format = "T81W1 native balanced ternary";
    mf.checksum = computed;
    mf.file_size = file_size;

    const uint8_t* cursor = payload.data();
    const uint8_t* end = payload.data() + payload.size();

    auto read_le64 = [&]() -> uint64_t {
        if (cursor + 8 > end) throw std::runtime_error("t81w: truncated metadata");
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(cursor[i]) << (8 * i);
        }
        cursor += 8;
        return value;
    };

    auto read_bytes = [&](size_t count, std::string& out) {
        if (cursor + count > end) throw std::runtime_error("t81w: truncated name");
        out.assign(reinterpret_cast<const char*>(cursor), count);
        cursor += count;
    };

    uint64_t num_tensors = read_le64();
    uint64_t zero_trits = 0;
    for (uint64_t ti = 0; ti < num_tensors; ++ti) {
        uint64_t name_len = read_le64();
        std::string name;
        read_bytes(name_len, name);
        uint64_t rank = read_le64();
        std::vector<uint64_t> shape;
        shape.reserve(rank);
        for (uint64_t r = 0; r < rank; ++r) {
            shape.push_back(read_le64());
        }
        uint64_t trits = read_le64();
        uint64_t limbs = (trits + 47) / 48;

        NativeTensor tensor;
        tensor.shape = shape;
        tensor.trits = trits;
        tensor.data.resize(limbs);
        for (uint64_t li = 0; li < limbs; ++li) {
            tensor.data[li] = read_le64();
        }

        zero_trits += count_zero_trits(tensor);

        TensorInfo info;
        info.name = name;
        info.shape = shape;
        info.num_trits = trits;
        mf.tensors.push_back(info);
        mf.total_trits += trits;
        mf.total_parameters += trits;
        mf.native.emplace(name, std::move(tensor));
    }

    if (mf.total_trits > 0) {
        mf.bits_per_trit = static_cast<double>(file_size * 8) / static_cast<double>(mf.total_trits);
        mf.sparsity = static_cast<double>(zero_trits) / static_cast<double>(mf.total_trits);
    }

    return mf;
}

void print_info(const ModelFile& mf) {
    uint64_t limbs = 0;
    for (const auto& [name, tensor] : mf.native) {
        limbs += tensor.padded_limbs();
    }
    std::cout << "Model contains " << mf.tensors.size() << " tensors, "
              << format_bytes(mf.total_trits / 5) << " avg density\n";
    std::cout << "Trits:        " << mf.total_trits << "\n";
    std::cout << "Limbs:        " << limbs << "\n";
    if (!mf.format.empty()) {
        std::cout << "Format:       " << mf.format << "\n";
    }
}

std::string format_bytes_impl(uint64_t bytes) {
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

std::string format_bytes(uint64_t bytes) {
    return format_bytes_impl(bytes);
}

std::string format_count(uint64_t value) {
    constexpr const char* const suffixes[] = {"", "K", "M", "B", "T"};
    double scaled = static_cast<double>(value);
    size_t idx = 0;
    while (idx + 1 < std::size(suffixes) && scaled >= 1000.0) {
        scaled /= 1000.0;
        ++idx;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f %s", scaled, suffixes[idx]);
    return buf;
}

namespace {

struct QuantTensorInfo {
    std::string name;
    std::vector<uint64_t> shape;
    std::string dtype;
    uint64_t data_offset = 0;
    uint64_t data_size = 0;
};

struct SafetensorFile {
    std::filesystem::path path;
    uint64_t header_size = 0;
    std::vector<uint8_t> header;
    std::vector<QuantTensorInfo> tensors;
};

inline float fp16_to_fp32(uint16_t h) {
    uint32_t sign = (h & 0x8000u) << 16;
    int32_t exp = (h & 0x7C00u) >> 10;
    uint32_t mant = h & 0x03FFu;
    if (exp == 0) {
        if (mant == 0) return std::bit_cast<float>(sign);
        exp = -14;
        do {
            exp++;
            mant <<= 1;
        } while ((mant & 0x0400u) == 0);
        mant &= 0x03FFu;
    } else if (exp == 31) {
        return std::bit_cast<float>(sign | 0x7F800000u | (mant << 13));
    } else {
        exp += (127 - 15);
    }
    uint32_t f = sign | (uint32_t(exp) << 23) | (mant << 13);
    return std::bit_cast<float>(f);
}

enum class Trit : int8_t { M = -1, Z = 0, P = 1 };

constexpr uint8_t trit_to_u3(Trit t) {
    return static_cast<uint8_t>(static_cast<int8_t>(t) + 1);
}

struct T3Block {
    float scale = 0.0f;
    uint8_t trits[48] = {};
};

void quantize_block_t3(const float* src, T3Block& block) {
    float amax = 0.0f;
    for (int i = 0; i < 128; ++i) {
        amax = std::max(amax, std::abs(src[i]));
    }
    block.scale = amax / 1.0f;
    uint32_t buffer = 0;
    int bits = 0;
    int out_idx = 0;
    for (int i = 0; i < 128; ++i) {
        float x = src[i] / (block.scale + 1e-8f);
        Trit t = (x > 0.5f) ? Trit::P : (x < -0.5f) ? Trit::M : Trit::Z;
        uint32_t val = trit_to_u3(t);
        buffer = (buffer << 3) | val;
        bits += 3;
        while (bits >= 8) {
            bits -= 8;
            block.trits[out_idx++] = uint8_t(buffer >> bits);
        }
    }
    if (bits > 0) {
        block.trits[out_idx++] = uint8_t(buffer << (8 - bits));
    }
}

std::vector<QuantTensorInfo> parse_safetensors_header(const std::vector<uint8_t>& header) {
    std::string text(header.begin(), header.end());
    JsonParser parser(text);
    JsonValue root = parser.parse();
    std::vector<QuantTensorInfo> tensors;
    for (const auto& [key, value] : root.object_value) {
        if (key.rfind("__", 0) == 0) continue;
        if (value.object_value.empty()) continue;
        auto dtype_it = value.object_value.find("dtype");
        if (dtype_it == value.object_value.end() || !dtype_it->second.is_string) continue;
        auto shape_it = value.object_value.find("shape");
        if (shape_it == value.object_value.end()) continue;
        auto offsets_it = value.object_value.find("data_offsets");
        auto lengths_it = value.object_value.find("data_lengths");
        if (offsets_it == value.object_value.end() || lengths_it == value.object_value.end()) continue;
        if (offsets_it->second.array_value.empty() || lengths_it->second.array_value.empty()) continue;
        QuantTensorInfo info;
        info.name = key;
        info.dtype = dtype_it->second.string_value;
        info.shape = json_to_shape(shape_it->second);
        info.data_offset = json_to_uint(offsets_it->second.array_value[0]);
        uint64_t length = json_to_uint(lengths_it->second.array_value[0]);
        info.data_size = length - info.data_offset;
        tensors.push_back(info);
    }
    return tensors;
}

struct ModelInfo {
    std::string arch = "llama";
    uint32_t n_layer = 0;
    uint32_t n_head = 0;
    uint32_t n_embd = 0;
    uint32_t context_length = 32768;
};

ModelInfo detect_model(const std::vector<QuantTensorInfo>& tensors) {
    ModelInfo info;
    for (const auto& t : tensors) {
        if (t.name.find("model.layers.") != std::string::npos) {
            size_t dot = t.name.find('.', 13);
            if (dot != std::string::npos) {
                uint32_t layer = 0;
                std::from_chars(t.name.data() + 13, t.name.data() + dot, layer);
                info.n_layer = std::max(info.n_layer, layer + 1);
            }
        }
        if (t.name.find("attn.q.weight") != std::string::npos ||
            t.name.find("self_attn.q_proj") != std::string::npos) {
            if (t.shape.size() >= 2) info.n_embd = static_cast<uint32_t>(t.shape[1]);
        }
        if (t.name.find("q_proj.weight") != std::string::npos && t.shape.size() == 2 && info.n_embd != 0) {
            info.n_head = static_cast<uint32_t>(t.shape[0] / info.n_embd);
        }
    }
    if (info.n_layer == 28 && info.n_embd == 4096) info.context_length = 131072;
    if (info.n_layer == 32 && info.n_embd == 4096) info.context_length = 131072;
    if (info.n_layer == 32 && info.n_embd == 5120) info.arch = "qwen2";
    return info;
}

struct GGUFWriter {
    std::vector<uint8_t> data;
    std::unordered_map<std::string, uint64_t> strings;

    void align(uint64_t a) {
        while (data.size() % a) data.push_back(0);
    }

    uint64_t add_string(const std::string& s) {
        if (auto it = strings.find(s); it != strings.end()) return it->second;
        uint64_t off = data.size();
        uint64_t len = s.size();
        for (int i = 0; i < 8; ++i) {
            data.push_back(static_cast<uint8_t>((len >> (8 * i)) & 0xFF));
        }
        data.insert(data.end(), s.begin(), s.end());
        align(32);
        strings[s] = off;
        return off;
    }

    void write_header(uint64_t tensor_count, uint64_t kv_count = 20) {
        uint64_t magic = 0x46554747ULL;
        uint32_t version = 3;
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((magic >> (8 * i)) & 0xFF));
        for (int i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((version >> (8 * i)) & 0xFF));
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((tensor_count >> (8 * i)) & 0xFF));
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((kv_count >> (8 * i)) & 0xFF));
    }

    void write_kv(const std::string& key, const std::string& value) {
        uint32_t vtype = 9;
        uint64_t koff = add_string(key);
        uint64_t voff = add_string(value);
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((koff >> (8 * i)) & 0xFF));
        for (int i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((vtype >> (8 * i)) & 0xFF));
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((voff >> (8 * i)) & 0xFF));
    }

    void write_kv(const std::string& key, uint32_t value) {
        uint32_t vtype = 2;
        uint64_t koff = add_string(key);
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((koff >> (8 * i)) & 0xFF));
        for (int i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((vtype >> (8 * i)) & 0xFF));
        for (int i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }

    void write_tensor(const std::string& name, const std::vector<uint64_t>& shape,
                      uint32_t type, uint64_t offset) {
        uint32_t n_dims = static_cast<uint32_t>(shape.size());
        for (int i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((n_dims >> (8 * i)) & 0xFF));
        for (uint64_t d : shape) {
            for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((d >> (8 * i)) & 0xFF));
        }
        for (int i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((type >> (8 * i)) & 0xFF));
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((offset >> (8 * i)) & 0xFF));
        uint64_t name_off = add_string(name);
        for (int i = 0; i < 8; ++i) data.push_back(static_cast<uint8_t>((name_off >> (8 * i)) & 0xFF));
    }
};

} // namespace

void quantize_safetensors_to_gguf(const std::filesystem::path& input,
                                   const std::filesystem::path& output) {
    if (!std::filesystem::exists(input)) {
        throw std::runtime_error("input path not found");
    }
    std::vector<SafetensorFile> files;
    auto add_file = [&](const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("cannot open " + path.string());
        uint64_t header_size = 0;
        f.read(reinterpret_cast<char*>(&header_size), sizeof(header_size));
        std::vector<uint8_t> header(header_size);
        f.read(reinterpret_cast<char*>(header.data()), header_size);
        SafetensorFile file;
        file.path = path;
        file.header_size = header_size;
        file.header = std::move(header);
        file.tensors = parse_safetensors_header(file.header);
        files.push_back(std::move(file));
    };
    if (std::filesystem::is_directory(input)) {
        for (const auto& entry : std::filesystem::directory_iterator(input)) {
            if (entry.path().extension() == ".safetensors") {
                add_file(entry.path());
            }
        }
    } else if (input.extension() == ".safetensors") {
        add_file(input);
    } else {
        throw std::runtime_error("input must be a .safetensors file or directory");
    }
    if (files.empty()) {
        throw std::runtime_error("no safetensors found in input");
    }
    std::vector<QuantTensorInfo> all_tensors;
    for (const auto& file : files) {
        all_tensors.insert(all_tensors.end(), file.tensors.begin(), file.tensors.end());
    }
    ModelInfo model = detect_model(all_tensors);
    GGUFWriter writer;
    writer.align(32);
    writer.write_header(0);
    writer.write_kv("general.architecture", model.arch);
    writer.write_kv("general.name", output.stem().string());
    writer.write_kv("general.file_type", uint32_t(32));
    writer.write_kv(model.arch + ".context_length", model.context_length);
    writer.write_kv(model.arch + ".block_count", model.n_layer);
    writer.write_kv(model.arch + ".embedding_length", model.n_embd);
    writer.write_kv(model.arch + ".attention.head_count", model.n_head);
    writer.write_kv("tokenizer.ggml.model", "llama");
    writer.write_kv("tokenizer.ggml.tokens", uint32_t(0));
    writer.write_kv("tokenizer.ggml.scores", uint32_t(0));
    writer.align(32);
    uint64_t tensor_count = 0;
    for (const auto& file : files) {
        std::ifstream f(file.path, std::ios::binary);
        if (!f) throw std::runtime_error("cannot reopen " + file.path.string());
        for (const auto& tensor : file.tensors) {
            if (tensor.shape.size() < 2) continue;
            uint64_t n_elements = 1;
            for (uint64_t dim : tensor.shape) {
                if (dim == 0) throw std::runtime_error("tensor dimension zero");
                n_elements *= dim;
            }
            std::vector<float> float_data(n_elements);
            f.seekg(8 + static_cast<std::streamoff>(file.header_size) + tensor.data_offset, std::ios::beg);
            if (tensor.dtype == "F16" || tensor.dtype == "BF16") {
                std::vector<uint16_t> raw(n_elements);
                f.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(n_elements * 2));
                for (uint64_t i = 0; i < n_elements; ++i) {
                    if (tensor.dtype == "F16") {
                        float_data[i] = fp16_to_fp32(raw[i]);
                    } else {
                        uint32_t bits = static_cast<uint32_t>(raw[i]) << 16;
                        float_data[i] = std::bit_cast<float>(bits);
                    }
                }
            } else if (tensor.dtype == "F32") {
                f.read(reinterpret_cast<char*>(float_data.data()), static_cast<std::streamsize>(n_elements * 4));
            } else {
                continue;
            }
            uint64_t tensor_offset = writer.data.size();
            std::vector<float> tmp(128);
            for (uint64_t idx = 0; idx < n_elements; idx += 128) {
                uint64_t block_count = std::min<uint64_t>(128, n_elements - idx);
                std::copy_n(float_data.data() + idx, block_count, tmp.data());
                if (block_count < 128) std::fill(tmp.begin() + block_count, tmp.end(), 0.0f);
                T3Block block{};
                quantize_block_t3(tmp.data(), block);
                writer.data.insert(writer.data.end(),
                                   reinterpret_cast<uint8_t*>(&block),
                                   reinterpret_cast<uint8_t*>(&block) + sizeof(T3Block));
            }
            std::vector<uint64_t> gguf_shape = tensor.shape;
            std::reverse(gguf_shape.begin(), gguf_shape.end());
            writer.write_tensor(tensor.name, gguf_shape, 99, tensor_offset);
            tensor_count++;
        }
    }
    uint64_t* count_ptr = reinterpret_cast<uint64_t*>(writer.data.data() + 16);
    *count_ptr = tensor_count;
    std::ofstream out(output, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write " + output.string());
    out.write(reinterpret_cast<const char*>(writer.data.data()), static_cast<std::streamsize>(writer.data.size()));
    uint64_t mb = writer.data.size() >> 20;
    std::cout << "Success! T3_K GGUF created: " << output << " (" << mb << " MB)\n";
    std::cout << "Run with llama.cpp (latest):\n";
    std::cout << "  ./llama-cli -m " << output << " -p \"Hello\" -n 512 --color\n";
    std::cout << "Note: You need llama.cpp with T3_K support (PR coming soon)\n";
}

} // namespace t81::weights
