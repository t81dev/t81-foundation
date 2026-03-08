#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "t81/isa/program.hpp"

namespace t81::weights {
struct ModelFile;
}

namespace t81::cli {

std::optional<t81::tisc::Program> build_program_from_source(
    const std::string& source, const std::string& diag_name,
    const std::shared_ptr<t81::weights::ModelFile>& weights_model = nullptr);

int compile(const std::filesystem::path& input, const std::filesystem::path& output,
            const std::string& source_override = {}, const std::string& source_name = {},
            std::shared_ptr<t81::weights::ModelFile> weights_model = nullptr);
int run_tisc(const std::filesystem::path& path,
             const std::optional<std::filesystem::path>& policy_path = std::nullopt,
             bool trace_enabled = false,
             const std::optional<std::filesystem::path>& trace_output_path = std::nullopt);
int disasm_tisc(const std::filesystem::path& path);
int debug_tisc(const std::filesystem::path& path,
               const std::optional<std::filesystem::path>& policy_path = std::nullopt);
int check_syntax(const std::filesystem::path& path);
int repl(const std::shared_ptr<t81::weights::ModelFile>& weights_model = nullptr,
         const std::optional<std::filesystem::path>& policy_path = std::nullopt,
         std::istream& input = std::cin);
int init_project(const std::string& name);
int init_package(const std::string& name);
int canonize_tensor(const std::string& input_file,
                    const std::filesystem::path& canonfs_root = ".t81_canonfs");
int canonfs_put_file(const std::filesystem::path& input,
                     const std::filesystem::path& canonfs_root = ".t81_canonfs");
int canonfs_list(const std::filesystem::path& canonfs_root = ".t81_canonfs", bool as_json = false);
int canonfs_get(const std::string& canonical_hash,
                const std::optional<std::filesystem::path>& output_path = std::nullopt,
                const std::filesystem::path& canonfs_root = ".t81_canonfs",
                bool as_json = false);
int canonfs_stat(const std::string& canonical_hash,
                 const std::filesystem::path& canonfs_root = ".t81_canonfs",
                 bool as_json = false);
int canonfs_verify(const std::string& canonical_hash,
                   const std::filesystem::path& canonfs_root = ".t81_canonfs",
                   bool as_json = false);
std::optional<std::string> canonfs_capture_snapshot_hash(
    const std::filesystem::path& canonfs_root = ".t81_canonfs",
    std::string* error_message = nullptr);
int canonfs_snapshot(const std::filesystem::path& canonfs_root = ".t81_canonfs",
                     bool as_json = false);
int canonfs_snapshot_diff(const std::string& lhs_snapshot_hash, const std::string& rhs_snapshot_hash,
                          const std::filesystem::path& canonfs_root = ".t81_canonfs",
                          bool as_json = false);
int canonfs_rollback(const std::string& snapshot_hash,
                     const std::filesystem::path& canonfs_root = ".t81_canonfs",
                     bool as_json = false);

int determinism_hash_file(const std::filesystem::path& input, bool as_json = false);
int determinism_hash_trace(const std::filesystem::path& input, bool as_json = false);
int determinism_diff_files(const std::filesystem::path& lhs, const std::filesystem::path& rhs,
                           bool as_json = false);
int determinism_diff_trace_files(const std::filesystem::path& lhs, const std::filesystem::path& rhs,
                                 bool as_json = false);

struct TraceArgs {
  std::string subcommand;
  std::vector<std::string> args;
  bool no_color = false;
};
int run_trace(const TraceArgs& args);

}  // namespace t81::cli
