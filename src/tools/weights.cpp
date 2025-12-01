#include "t81/weights.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
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
} // namespace

ModelFile load_gguf(const std::filesystem::path&) {
    throw std::runtime_error("GGUF loader is not implemented yet");
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
        if (key.starts_with("__")) continue;
        if (!value.object_value.contains("dtype")) continue;
        const auto& dtype = value.object_value.at("dtype");
        if (!dtype.is_string || dtype.string_value != "I8") continue;
        const auto& shape_val = value.object_value.at("shape");
        auto shape = json_to_shape(shape_val);
        uint64_t count = product_of(shape);

        const auto& offsets_val = value.object_value.at("data_offsets");
        const auto& lengths_val = value.object_value.at("data_lengths");
        if (offsets_val.array_value.empty() || lengths_val.array_value.empty()) {
            throw std::runtime_error("SafeTensors: missing offsets/lengths");
        }
        uint64_t offset = json_to_uint(offsets_val.array_value[0]);
        uint64_t length = json_to_uint(lengths_val.array_value[0]);
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
