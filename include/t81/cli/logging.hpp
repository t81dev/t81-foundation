#pragma once

#include <iostream>
#include <string_view>

struct Flags {
    bool verbose = false;
    bool quiet = false;
};

inline Flags g_flags;

inline void verbose(std::string_view msg) {
    if (g_flags.verbose) {
        std::cerr << "[verbose] " << msg << '\n';
    }
}

inline void info(std::string_view msg) {
    if (!g_flags.quiet) {
        std::cout << msg << '\n';
    }
}

inline void error(std::string_view msg) {
    if (!g_flags.quiet) {
        std::cerr << "error: " << msg << '\n';
    }
}
