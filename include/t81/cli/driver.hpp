#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace t81::weights {
struct ModelFile;
}

namespace t81::cli {

int compile(const std::filesystem::path& input,
            const std::filesystem::path& output,
            const std::string& source_override = {},
            const std::string& source_name = {},
            std::shared_ptr<t81::weights::ModelFile> weights_model = nullptr);
int run_tisc(const std::filesystem::path& path);
int check_syntax(const std::filesystem::path& path);
int repl(const std::shared_ptr<t81::weights::ModelFile>& weights_model = nullptr,
         std::istream& input = std::cin);

} // namespace t81::cli
