#include "t81/weights.hpp"

#include <iomanip>
#include <sstream>

namespace t81::weights {

namespace {
std::string escape_string(const std::string& input) {
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '\\': oss << "\\\\"; break;
            case '\"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\x" << std::hex << std::uppercase << std::setw(2)
                        << std::setfill('0') << (static_cast<int>(c) & 0xFF)
                        << std::dec << std::nouppercase;
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

std::string join_shape(const std::vector<uint64_t>& shape) {
    std::ostringstream oss;
    if (shape.empty()) {
        oss << "[]";
    } else {
        oss << "[";
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i != 0) oss << ", ";
            oss << shape[i];
        }
        oss << "]";
    }
    return oss.str();
}

} // namespace

std::string emit_t81w_module(const ModelFile& mf, const std::string& source_name) {
    std::ostringstream oss;
    oss << "// Generated from " << source_name << "\n";
    oss << "let __t81w_checksum: String = \"" << escape_string(mf.checksum) << "\";\n";
    oss << "let __t81w_tensor_count: i32 = " << static_cast<int64_t>(mf.tensors.size()) << ";\n";
    for (size_t idx = 0; idx < mf.tensors.size(); ++idx) {
        const auto& tensor = mf.tensors[idx];
        oss << "let __t81w_tensor_" << idx << "_name: String = \""
            << escape_string(tensor.name) << "\";\n";
        oss << "let __t81w_tensor_" << idx << "_trits: i32 = "
            << static_cast<int64_t>(tensor.num_trits) << ";\n";
        oss << "let __t81w_tensor_" << idx << "_shape: String = \""
            << escape_string(join_shape(tensor.shape)) << "\";\n";
    }
    oss << "\nfn __t81w_module_entry() -> i32 {\n"
        "    return 0;\n"
        "}\n";
    return oss.str();
}

} // namespace t81::weights
