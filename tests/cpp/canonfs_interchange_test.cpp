#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "t81/canonfs/interchange.hpp"
#include "t81/canonfs/interchange_ops.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "canonfs_interchange_test failure: " << message << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  using namespace t81::canonfs::interchange;

  const std::vector<Entry> entries = {
      Entry{"a.txt", "sha3-256:abc123", 12},
      Entry{"nested/b.txt", "sha3-256:def456", 34},
  };

  const std::string manifest_text =
      render_manifest("host-directory", "/tmp/input-tree", entries);
  std::string error_message;
  if (!expect(validate_manifest_document(manifest_text, error_message),
              "rendered manifest should validate")) {
    return 1;
  }

  Manifest parsed_manifest;
  if (!expect(parse_manifest(manifest_text, parsed_manifest, error_message),
              "rendered manifest should parse")) {
    return 1;
  }
  if (!expect(parsed_manifest.entries.size() == 2, "parsed manifest entry count mismatch")) {
    return 1;
  }
  if (!expect(parsed_manifest.entries[0].path == "a.txt", "first parsed manifest path mismatch")) {
    return 1;
  }
  if (!expect(parsed_manifest.entries[1].object_ref == "sha3-256:def456",
              "second parsed manifest ref mismatch")) {
    return 1;
  }

  const std::string import_provenance = render_import_provenance(
      "host-directory", "/tmp/input-tree", {"sha3-256:abc123", "sha3-256:def456"},
      "sha3-256:manifest001");
  if (!expect(validate_import_provenance_document(import_provenance, error_message),
              "import provenance should validate")) {
    return 1;
  }

  const std::string export_provenance = render_export_provenance(
      {"sha3-256:abc123", "sha3-256:def456"}, "host-directory", "/tmp/output-tree",
      "sha3-256:manifest001");
  if (!expect(validate_export_provenance_document(export_provenance, error_message),
              "export provenance should validate")) {
    return 1;
  }

  const std::string import_result = render_import_result(
      "partial", "host-directory", "/tmp/input-tree", {"sha3-256:abc123"},
      "sha3-256:prov001", "sha3-256:manifest001", {"a.txt"}, {},
      {"policy denied import of nested/b.txt: hash not allowed"}, "partial");
  if (!expect(validate_import_result_document(import_result, error_message),
              "import result should validate")) {
    return 1;
  }

  const std::string export_result = render_export_result(
      "ok", {"sha3-256:abc123", "sha3-256:def456"}, "host-directory", "/tmp/output-tree",
      "sha3-256:prov002", "sha3-256:manifest001", {"a.txt", "nested/b.txt"}, {}, {}, "allowed");
  if (!expect(validate_export_result_document(export_result, error_message),
              "export result should validate")) {
    return 1;
  }

  const std::string invalid_manifest =
      "{\n"
      "  \"schema\": \"t81.canonfs-interchange-manifest.v1\",\n"
      "  \"source_kind\": \"host-directory\",\n"
      "  \"source_ref\": \"/tmp/input-tree\",\n"
      "  \"entries\": [\n"
      "    {\"path\": \"bad.txt\", \"object_ref\": \"invalid-ref\", \"size_bytes\": 1}\n"
      "  ]\n"
      "}\n";
  if (!expect(!validate_manifest_document(invalid_manifest, error_message),
              "invalid manifest should fail validation")) {
    return 1;
  }

  const std::string invalid_export_result =
      "{\n"
      "  \"schema\": \"t81.canonfs-export.v1\",\n"
      "  \"status\": \"ok\",\n"
      "  \"source_objects\": [\"sha3-256:abc123\"],\n"
      "  \"target_kind\": \"host-file\",\n"
      "  \"target_ref\": \"/tmp/out.bin\",\n"
      "  \"provenance_ref\": \"sha3-256:prov002\",\n"
      "  \"warnings\": [],\n"
      "  \"errors\": [{\"kind\": \"wrong-kind\", \"message\": \"bad\", \"code\": \"x\"}],\n"
      "  \"policy_result\": \"allowed\",\n"
      "  \"materialization_summary\": {\n"
      "    \"timestamps\": \"not-restored\",\n"
      "    \"ownership\": \"synthesized\",\n"
      "    \"mode_hint\": \"preserved\"\n"
      "  }\n"
      "}\n";
  if (!expect(!validate_export_result_document(invalid_export_result, error_message),
              "invalid export result should fail validation")) {
    return 1;
  }

  {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "canonfs-interchange-ops-test";
    const fs::path source = root / "payload.txt";
    const fs::path restored = root / "restored.txt";
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    if (!expect(!ec, "ops test root create failed")) return 1;

    {
      std::ofstream out(source);
      out << "canonfs-interchange-ops";
    }

    t81::canonfs::ImportOptions import_options;
    import_options.canonfs_root = root / ".t81_canonfs";
    const auto import_outcome = t81::canonfs::import_path(source, import_options);
    if (!expect(import_outcome.ok(), "core import_path should succeed")) return 1;
    if (!expect(import_outcome.imported_objects.size() == 1, "core import_path object count mismatch")) {
      return 1;
    }

    t81::canonfs::ExportOptions export_options;
    export_options.canonfs_root = import_options.canonfs_root;
    const auto export_outcome =
        t81::canonfs::export_ref(import_outcome.imported_objects.front(), restored, export_options);
    if (!expect(export_outcome.ok(), "core export_ref should succeed")) return 1;

    std::ifstream in(restored);
    std::string restored_text;
    std::getline(in, restored_text);
    if (!expect(restored_text == "canonfs-interchange-ops", "core export_ref payload mismatch")) {
      return 1;
    }

    export_options.policy_evaluator = [](std::string_view, std::string_view, std::string_view) {
      return t81::canonfs::InterchangePolicyDecision{.allowed = false, .reason = "blocked"};
    };
    const auto denied_export =
        t81::canonfs::export_ref(import_outcome.imported_objects.front(), root / "denied.txt",
                                 export_options);
    if (!expect(!denied_export.ok(), "policy-denied core export should fail")) return 1;
    if (!expect(denied_export.policy_result == "denied", "policy-denied export result mismatch")) {
      return 1;
    }

    fs::remove_all(root, ec);
    if (!expect(!ec, "ops test root cleanup failed")) return 1;
  }

  return 0;
}
