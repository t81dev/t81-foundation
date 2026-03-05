#pragma once

#include <optional>
#include <vector>
#include <string>

namespace t81::codec {

struct GGUFHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t tensor_count;
    uint32_t kv_count;
};

struct GGUTensor {
    std::string name;
    std::vector<uint32_t> dimensions;
    std::vector<float> data;
    uint32_t type;
};

class GGUFParser {
public:
    static bool is_gguf_file(const std::string& path);
    static std::optional<std::vector<GGUTensor>> parse_model(const std::string& path);
};

} // namespace t81::codec
