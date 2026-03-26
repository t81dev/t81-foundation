#include "t81/canonfs/interchange_ops.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/interchange.hpp"
#include "t81/crypto/sha3.hpp"

namespace fs = std::filesystem;

namespace {

bool is_symlink_path(const fs::path& path) {
  std::error_code ec;
  return fs::is_symlink(fs::symlink_status(path, ec));
}

std::vector<std::byte> read_file_bytes(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  std::vector<std::byte> out;
  char ch = 0;
  while (in.get(ch)) {
    out.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return out;
}

std::string bytes_to_string(const std::vector<std::byte>& bytes) {
  std::string text;
  text.reserve(bytes.size());
  for (std::byte b : bytes) {
    text.push_back(static_cast<char>(std::to_integer<unsigned char>(b)));
  }
  return text;
}

std::vector<std::byte> string_to_bytes(std::string_view text) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (char c : text) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
  }
  return bytes;
}

std::string canonfs_ref_from_bytes(std::span<const std::byte> bytes) {
  return "sha3-256:" + t81::hash::hash_bytes(bytes).to_string();
}

std::optional<std::string> write_canonfs_raw_object(const fs::path& canonfs_root,
                                                    std::span<const std::byte> bytes,
                                                    std::string& error_message) {
  auto driver = t81::canonfs::make_persistent_driver(canonfs_root);
  auto write_res = driver->write_object(t81::canonfs::ObjectType::RawBlock, bytes);
  if (!write_res) {
    error_message = "failed to write CanonFS object";
    return std::nullopt;
  }
  return "sha3-256:" + write_res->hash.h.to_string();
}

bool write_validated_canonfs_json_object(
    const fs::path& canonfs_root, std::string_view object_label, std::string_view text,
    bool (*validator)(std::string_view, std::string&), std::string& error_message,
    std::optional<std::string>* object_ref = nullptr) {
  std::string validation_error;
  if (!validator(text, validation_error)) {
    error_message = std::string(object_label) + " failed RFC-00D1 validation: " + validation_error;
    return false;
  }
  const auto bytes = string_to_bytes(text);
  const auto write_ref =
      write_canonfs_raw_object(canonfs_root, std::span<const std::byte>(bytes.data(), bytes.size()),
                               error_message);
  if (!write_ref) {
    return false;
  }
  if (object_ref) {
    *object_ref = *write_ref;
  }
  return true;
}

struct CanonFsObjectInfo {
  std::string hash;
  std::uintmax_t size_bytes = 0;
};

bool load_canonfs_object(const fs::path& canonfs_root, std::string_view prefixed_hash,
                         std::vector<std::byte>& bytes, CanonFsObjectInfo& info,
                         std::string& error_message) {
  constexpr std::string_view prefix = "sha3-256:";
  if (prefixed_hash.rfind(prefix, 0) != 0 || prefixed_hash.size() <= prefix.size()) {
    error_message = "canonical hash must start with sha3-256:";
    return false;
  }
  const std::string bare_hash(prefixed_hash.substr(prefix.size()));
  auto driver = t81::canonfs::make_persistent_driver(canonfs_root);
  t81::canonfs::CanonHash hash;
  try {
    hash.h = t81::hash::CanonHash81::from_string(bare_hash);
  } catch (const std::exception&) {
    error_message = "invalid CanonFS hash: " + std::string(prefixed_hash);
    return false;
  }
  t81::canonfs::CanonRef ref{hash};
  auto read_res = driver->read_object_bytes(ref);
  if (!read_res) {
    error_message = "object not found: " + std::string(prefixed_hash);
    return false;
  }
  bytes = std::move(read_res.value());
  info.hash = std::string(prefixed_hash);
  info.size_bytes = bytes.size();
  return true;
}

bool is_safe_relative_export_path(const fs::path& path) {
  if (path.is_absolute()) {
    return false;
  }
  for (const auto& part : path) {
    if (part == "..") {
      return false;
    }
  }
  return true;
}

bool allow_by_policy(t81::canonfs::InterchangePolicyProfile profile,
                     const t81::canonfs::InterchangePolicyEvaluator& evaluator,
                     std::string_view canonical_ref, std::string_view operation,
                     std::string_view path, bool& saw_policy_denial, std::string& reason) {
  const bool is_import = operation == "canonfs.import";
  const bool is_export = operation == "canonfs.export";
  switch (profile) {
    case t81::canonfs::InterchangePolicyProfile::Permissive:
      break;
    case t81::canonfs::InterchangePolicyProfile::ImportOnly:
      if (is_export) {
        saw_policy_denial = true;
        reason = "policy-profile import-only denies canonfs.export";
        return false;
      }
      break;
    case t81::canonfs::InterchangePolicyProfile::ExportOnly:
      if (is_import) {
        saw_policy_denial = true;
        reason = "policy-profile export-only denies canonfs.import";
        return false;
      }
      break;
    case t81::canonfs::InterchangePolicyProfile::DenyAll:
      saw_policy_denial = true;
      reason = "policy-profile deny-all denies " + std::string(operation);
      return false;
  }
  if (!evaluator) {
    reason = "allow";
    return true;
  }
  const auto verdict = evaluator(canonical_ref, operation, path);
  reason = verdict.reason.empty() ? "allow" : verdict.reason;
  if (!verdict.allowed) {
    saw_policy_denial = true;
  }
  return verdict.allowed;
}

bool collect_import_entries(const fs::path& input,
                            std::vector<t81::canonfs::interchange::Entry>& entries,
                            std::vector<std::string>& imported_objects,
                            std::vector<std::string>& imported_paths,
                            std::vector<std::string>& warnings,
                            std::vector<std::string>& errors, const fs::path& canonfs_root,
                            t81::canonfs::InterchangePolicyProfile policy_profile,
                            const t81::canonfs::InterchangePolicyEvaluator& policy_evaluator,
                            bool& saw_policy_denial) {
  if (is_symlink_path(input)) {
    errors.push_back("symlinks are not supported by canonfs import v1: " + input.string());
    return false;
  }
  if (fs::is_regular_file(input)) {
    auto bytes = read_file_bytes(input);
    const std::string object_ref =
        canonfs_ref_from_bytes(std::span<const std::byte>(bytes.data(), bytes.size()));
    std::string policy_reason;
    if (!allow_by_policy(policy_profile, policy_evaluator, object_ref, "canonfs.import",
                         input.filename().generic_string(), saw_policy_denial, policy_reason)) {
      errors.push_back("policy denied import of " + input.filename().generic_string() + ": " +
                       policy_reason);
      return false;
    }
    std::string write_error;
    const auto write_ref = write_canonfs_raw_object(
        canonfs_root, std::span<const std::byte>(bytes.data(), bytes.size()), write_error);
    if (!write_ref) {
      errors.push_back(write_error);
      return false;
    }
    t81::canonfs::interchange::Entry entry;
    entry.path = input.filename().generic_string();
    entry.object_ref = *write_ref;
    entry.size_bytes = bytes.size();
    entries.push_back(entry);
    imported_objects.push_back(*write_ref);
    imported_paths.push_back(entry.path);
    return true;
  }
  if (!fs::is_directory(input)) {
    errors.push_back("unsupported import source: " + input.string());
    return false;
  }
  for (auto it = fs::recursive_directory_iterator(input); it != fs::recursive_directory_iterator(); ++it) {
    const fs::path path = it->path();
    const fs::path rel = fs::relative(path, input);
    if (is_symlink_path(path)) {
      errors.push_back("symlinks are not supported by canonfs import v1: " + rel.generic_string());
      if (it->is_directory()) {
        it.disable_recursion_pending();
      }
      continue;
    }
    if (it->is_directory()) {
      continue;
    }
    if (!it->is_regular_file()) {
      warnings.push_back("skipped non-regular path: " + rel.generic_string());
      continue;
    }
    auto bytes = read_file_bytes(path);
    const std::string object_ref =
        canonfs_ref_from_bytes(std::span<const std::byte>(bytes.data(), bytes.size()));
    std::string policy_reason;
    if (!allow_by_policy(policy_profile, policy_evaluator, object_ref, "canonfs.import",
                         rel.generic_string(), saw_policy_denial, policy_reason)) {
      errors.push_back("policy denied import of " + rel.generic_string() + ": " + policy_reason);
      continue;
    }
    std::string write_error;
    const auto write_ref = write_canonfs_raw_object(
        canonfs_root, std::span<const std::byte>(bytes.data(), bytes.size()), write_error);
    if (!write_ref) {
      errors.push_back(rel.generic_string() + ": " + write_error);
      continue;
    }
    t81::canonfs::interchange::Entry entry;
    entry.path = rel.generic_string();
    entry.object_ref = *write_ref;
    entry.size_bytes = bytes.size();
    entries.push_back(entry);
    imported_objects.push_back(*write_ref);
    imported_paths.push_back(entry.path);
  }
  std::sort(entries.begin(), entries.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.path < rhs.path; });
  std::sort(imported_objects.begin(), imported_objects.end());
  std::sort(imported_paths.begin(), imported_paths.end());
  return !entries.empty();
}

}  // namespace

namespace t81::canonfs {

std::string_view interchange_policy_profile_name(InterchangePolicyProfile profile) {
  switch (profile) {
    case InterchangePolicyProfile::Permissive:
      return "permissive";
    case InterchangePolicyProfile::ImportOnly:
      return "import-only";
    case InterchangePolicyProfile::ExportOnly:
      return "export-only";
    case InterchangePolicyProfile::DenyAll:
      return "deny-all";
  }
  return "permissive";
}

std::optional<InterchangePolicyProfile> parse_interchange_policy_profile(
    std::string_view profile_name) {
  if (profile_name == "permissive") {
    return InterchangePolicyProfile::Permissive;
  }
  if (profile_name == "import-only") {
    return InterchangePolicyProfile::ImportOnly;
  }
  if (profile_name == "export-only") {
    return InterchangePolicyProfile::ExportOnly;
  }
  if (profile_name == "deny-all") {
    return InterchangePolicyProfile::DenyAll;
  }
  return std::nullopt;
}

ImportOutcome import_path(const fs::path& input, const ImportOptions& options) {
  ImportOutcome outcome;
  outcome.source_kind = fs::is_directory(input) ? "host-directory" : "host-file";
  outcome.source_ref = input.string();
  if (!fs::exists(input)) {
    outcome.status = "error";
    outcome.errors.push_back("input path does not exist: " + input.string());
    outcome.policy_result = "denied";
    return outcome;
  }

  std::vector<t81::canonfs::interchange::Entry> entries;
  bool saw_policy_denial = false;
  const bool has_entries = collect_import_entries(input, entries, outcome.imported_objects,
                                                  outcome.imported_paths, outcome.warnings,
                                                  outcome.errors, options.canonfs_root,
                                                  options.policy_profile,
                                                  options.policy_evaluator, saw_policy_denial);
  if (has_entries) {
    const auto manifest_text =
        t81::canonfs::interchange::render_manifest(outcome.source_kind, outcome.source_ref, entries);
    std::string manifest_error;
    std::optional<std::string> manifest_result;
    if (!write_validated_canonfs_json_object(
            options.canonfs_root, "import manifest", manifest_text,
            t81::canonfs::interchange::validate_manifest_document, manifest_error, &manifest_result)) {
      outcome.errors.push_back("failed to write import manifest: " + manifest_error);
    } else {
      outcome.manifest_ref = *manifest_result;
    }
  }

  const auto provenance_text = t81::canonfs::interchange::render_import_provenance(
      outcome.source_kind, outcome.source_ref, outcome.imported_objects, outcome.manifest_ref);
  std::string provenance_error;
  std::optional<std::string> provenance_result;
  if (!write_validated_canonfs_json_object(options.canonfs_root, "import provenance",
                                           provenance_text,
                                           t81::canonfs::interchange::validate_import_provenance_document,
                                           provenance_error, &provenance_result)) {
    outcome.errors.push_back("failed to write import provenance: " + provenance_error);
  } else {
    outcome.provenance_ref = *provenance_result;
  }

  outcome.status = outcome.imported_objects.empty() ? "error"
                                                    : (outcome.errors.empty() ? "ok" : "partial");
  outcome.policy_result =
      saw_policy_denial ? (outcome.imported_objects.empty() ? "denied" : "partial") : "allowed";
  return outcome;
}

ExportOutcome export_ref(const std::string& canonical_hash, const fs::path& output_path,
                         const ExportOptions& options) {
  ExportOutcome outcome;
  outcome.target_kind = "host-file";
  outcome.target_ref = output_path.string();

  std::vector<std::byte> bytes;
  CanonFsObjectInfo info;
  std::string error_message;
  if (!load_canonfs_object(options.canonfs_root, canonical_hash, bytes, info, error_message)) {
    outcome.status = "error";
    outcome.source_objects = {canonical_hash};
    outcome.errors.push_back(error_message);
    outcome.policy_result = "denied";
    return outcome;
  }

  bool saw_policy_denial = false;
  t81::canonfs::interchange::Manifest manifest;
  std::string manifest_error;
  const std::string payload_text = bytes_to_string(bytes);
  const bool is_manifest_document = t81::canonfs::interchange::is_manifest_document(payload_text);
  if (t81::canonfs::interchange::parse_manifest(payload_text, manifest, manifest_error)) {
    outcome.target_kind = "host-directory";
    outcome.manifest_ref = canonical_hash;
    outcome.source_objects.reserve(manifest.entries.size());
    std::error_code ec;
    fs::create_directories(output_path, ec);
    if (ec) {
      outcome.errors.push_back("could not create export directory: " + output_path.string());
    } else {
      for (const auto& entry : manifest.entries) {
        outcome.source_objects.push_back(entry.object_ref);
        std::string policy_reason;
        if (!allow_by_policy(options.policy_profile, options.policy_evaluator, entry.object_ref,
                             "canonfs.export", entry.path, saw_policy_denial, policy_reason)) {
          outcome.errors.push_back("policy denied export of " + entry.path + ": " + policy_reason);
          continue;
        }
        const fs::path relative_path(entry.path);
        if (!is_safe_relative_export_path(relative_path)) {
          outcome.errors.push_back("unsafe export path in manifest: " + entry.path);
          continue;
        }
        std::vector<std::byte> entry_bytes;
        CanonFsObjectInfo entry_info;
        std::string entry_error;
        if (!load_canonfs_object(options.canonfs_root, entry.object_ref, entry_bytes, entry_info,
                                 entry_error)) {
          outcome.errors.push_back(entry.path + ": " + entry_error);
          continue;
        }
        const fs::path target_file = output_path / relative_path;
        fs::create_directories(target_file.parent_path(), ec);
        if (ec) {
          outcome.errors.push_back("could not create export directory: " +
                                   target_file.parent_path().string());
          continue;
        }
        std::ofstream out(target_file, std::ios::binary | std::ios::trunc);
        if (!out) {
          outcome.errors.push_back("could not open output file: " + target_file.string());
          continue;
        }
        for (std::byte b : entry_bytes) {
          out.put(static_cast<char>(std::to_integer<unsigned char>(b)));
        }
        if (!out.good()) {
          outcome.errors.push_back("failed writing output file: " + target_file.string());
          continue;
        }
        outcome.materialized_paths.push_back(entry.path);
      }
    }
  } else if (!is_manifest_document) {
    outcome.source_objects.push_back(canonical_hash);
    std::string policy_reason;
    if (!allow_by_policy(options.policy_profile, options.policy_evaluator, canonical_hash,
                         "canonfs.export",
                         output_path.filename().generic_string(), saw_policy_denial,
                         policy_reason)) {
      outcome.errors.push_back("policy denied export: " + policy_reason);
    }
    std::error_code ec;
    if (!output_path.parent_path().empty() && outcome.errors.empty()) {
      fs::create_directories(output_path.parent_path(), ec);
    }
    if (ec) {
      outcome.errors.push_back("could not create export directory: " +
                               output_path.parent_path().string());
    } else if (outcome.errors.empty()) {
      std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
      if (!out) {
        outcome.errors.push_back("could not open output file: " + output_path.string());
      } else {
        for (std::byte b : bytes) {
          out.put(static_cast<char>(std::to_integer<unsigned char>(b)));
        }
        if (!out.good()) {
          outcome.errors.push_back("failed writing output file: " + output_path.string());
        } else {
          outcome.materialized_paths.push_back(output_path.filename().generic_string());
        }
      }
    }
  } else {
    outcome.source_objects.push_back(canonical_hash);
    outcome.errors.push_back("invalid interchange manifest: " + manifest_error);
  }

  const auto provenance_text = t81::canonfs::interchange::render_export_provenance(
      outcome.source_objects, outcome.target_kind, outcome.target_ref, outcome.manifest_ref);
  std::string provenance_error;
  std::optional<std::string> provenance_result;
  if (!write_validated_canonfs_json_object(
          options.canonfs_root, "export provenance", provenance_text,
          t81::canonfs::interchange::validate_export_provenance_document, provenance_error,
          &provenance_result)) {
    outcome.errors.push_back("failed to write export provenance: " + provenance_error);
  } else {
    outcome.provenance_ref = *provenance_result;
  }

  outcome.status = outcome.materialized_paths.empty() ? "error"
                                                      : (outcome.errors.empty() ? "ok" : "partial");
  outcome.policy_result = saw_policy_denial
                              ? (outcome.materialized_paths.empty() ? "denied" : "partial")
                              : "allowed";
  return outcome;
}

}  // namespace t81::canonfs
