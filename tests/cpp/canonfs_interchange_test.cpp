#include <iostream>
#include <string>
#include <vector>

#include "t81/canonfs/interchange.hpp"

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

  return 0;
}
