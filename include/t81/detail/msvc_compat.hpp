// include/t81/detail/msvc_compat.hpp
#pragma once
#include <string_view>

inline std::string_view make_sv(const char* begin, const char* end) noexcept {
    return std::string_view(begin, static_cast<std::size_t>(end - begin));
}

inline std::string make_str(const char* begin, const char* end) {
    return std::string(begin, static_cast<std::size_t>(end - begin));
}
