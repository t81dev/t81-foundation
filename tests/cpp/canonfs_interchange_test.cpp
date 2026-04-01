#include <filesystem>
#include <fstream>
#include <iostream>
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

t81::canonfs::interchange::Issue issue(std::string_view reason, std::string_view message) {
  return t81::canonfs::interchange::Issue{std::string(reason), std::string(message)};
}

}  // namespace

int main() {
  using namespace t81::canonfs::interchange;

  const std::vector<Entry> entries = {
      Entry{"a.txt", "sha3-256:abc123", 12},
      Entry{"nested/b.txt", "sha3-256:def456", 34},
  };

  const std::string manifest_text = render_manifest("host-directory", "/tmp/input-tree", entries);
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
      "sha3-256:manifest001", "permissive");
  if (!expect(validate_import_provenance_document(import_provenance, error_message),
              "import provenance should validate")) {
    return 1;
  }

  const std::string export_provenance =
      render_export_provenance({"sha3-256:abc123", "sha3-256:def456"}, "host-directory",
                               "/tmp/output-tree", "sha3-256:manifest001", "permissive");
  if (!expect(validate_export_provenance_document(export_provenance, error_message),
              "export provenance should validate")) {
    return 1;
  }

  const std::string import_result = render_import_result(
      "partial", "host-directory", "/tmp/input-tree", {"sha3-256:abc123"}, "sha3-256:prov001",
      "sha3-256:manifest001", {"a.txt"}, {},
      {issue("policy_denied", "policy denied import of nested/b.txt: hash not allowed")},
      "partial", "permissive");
  if (!expect(validate_import_result_document(import_result, error_message),
              "import result should validate")) {
    return 1;
  }
  if (!expect(import_result.find("\"kind\": \"policy-failure\"") != std::string::npos,
              "import result should classify policy denials")) {
    return 1;
  }
  if (!expect(import_result.find("\"code\": \"canonfs-policy-denied\"") != std::string::npos,
              "import result should emit policy denial code")) {
    return 1;
  }
  if (!expect(import_result.find("\"reason\": \"policy_denied\"") != std::string::npos,
              "import result should emit explicit reason")) {
    return 1;
  }

  const std::string import_normalization_failure = render_import_result(
      "error", "host-directory", "/tmp/input-tree", {}, "", "", {}, {},
      {issue("symlink_not_supported",
             "symlinks are not supported by canonfs import v1: nested/link")},
      "denied", "permissive");
  if (!expect(validate_import_result_document(import_normalization_failure, error_message),
              "import normalization failure result should validate")) {
    return 1;
  }
  if (!expect(import_normalization_failure.find("\"kind\": \"normalization-failure\"") !=
                  std::string::npos,
              "import result should classify normalization failures")) {
    return 1;
  }
  if (!expect(import_normalization_failure.find(
                  "\"code\": \"canonfs-import-symlink-not-supported\"") != std::string::npos,
              "import result should emit normalization failure code")) {
    return 1;
  }

  const std::string import_missing_source = render_import_result(
      "error", "host-file", "/tmp/missing-input.bin", {}, "", "", {}, {},
      {issue("missing_source", "input path does not exist: /tmp/missing-input.bin")}, "allowed",
      "permissive");
  if (!expect(validate_import_result_document(import_missing_source, error_message),
              "import missing source result should validate")) {
    return 1;
  }
  if (!expect(import_missing_source.find("\"kind\": \"source-failure\"") != std::string::npos,
              "import result should classify missing source as source failure")) {
    return 1;
  }
  if (!expect(import_missing_source.find("\"code\": \"canonfs-import-missing-source\"") !=
                  std::string::npos,
              "import result should emit missing source code")) {
    return 1;
  }

  const std::string export_result =
      render_export_result("ok", {"sha3-256:abc123", "sha3-256:def456"}, "host-directory",
                           "/tmp/output-tree", "sha3-256:prov002", "sha3-256:manifest001",
                           {"a.txt", "nested/b.txt"}, {}, {}, "allowed", "permissive");
  if (!expect(validate_export_result_document(export_result, error_message),
              "export result should validate")) {
    return 1;
  }

  const std::string export_target_failure = render_export_result(
      "error", {"sha3-256:abc123"}, "host-file", "/tmp/output-tree", "sha3-256:prov003", "",
      {}, {}, {issue("target_open_failed", "could not open output file: /tmp/output-tree")},
      "allowed", "permissive");
  if (!expect(validate_export_result_document(export_target_failure, error_message),
              "export target failure result should validate")) {
    return 1;
  }
  if (!expect(export_target_failure.find("\"kind\": \"target-failure\"") != std::string::npos,
              "export result should classify target failures")) {
    return 1;
  }
  if (!expect(export_target_failure.find("\"code\": \"canonfs-export-target-open-failed\"") !=
                  std::string::npos,
              "export result should emit target failure code")) {
    return 1;
  }

  const std::string export_unsafe_path = render_export_result(
      "error", {"sha3-256:abc123"}, "host-directory", "/tmp/output-tree", "sha3-256:prov004",
      "sha3-256:manifest004", {}, {},
      {issue("unsafe_target_path", "unsafe export path in manifest: ../escape.txt")}, "allowed",
      "permissive");
  if (!expect(validate_export_result_document(export_unsafe_path, error_message),
              "export unsafe path result should validate")) {
    return 1;
  }
  if (!expect(export_unsafe_path.find("\"kind\": \"target-failure\"") != std::string::npos,
              "export result should classify unsafe export path as target failure")) {
    return 1;
  }
  if (!expect(export_unsafe_path.find("\"code\": \"canonfs-export-unsafe-target-path\"") !=
                  std::string::npos,
              "export result should emit unsafe target path code")) {
    return 1;
  }

  const std::string export_missing_object = render_export_result(
      "error", {"sha3-256:missing"}, "host-file", "/tmp/out.bin", "sha3-256:prov005", "", {}, {},
      {issue("missing_object", "object not found: sha3-256:missing")}, "allowed", "permissive");
  if (!expect(validate_export_result_document(export_missing_object, error_message),
              "export missing object result should validate")) {
    return 1;
  }
  if (!expect(export_missing_object.find("\"kind\": \"materialization-failure\"") !=
                  std::string::npos,
              "export result should classify missing object as materialization failure")) {
    return 1;
  }
  if (!expect(export_missing_object.find("\"code\": \"canonfs-export-missing-object\"") !=
                  std::string::npos,
              "export result should emit missing object code")) {
    return 1;
  }

  const std::string export_invalid_schema = render_export_result(
      "error", {"sha3-256:weird"}, "host-file", "/tmp/out.bin", "", "", {}, {},
      {issue("invalid_schema", "invalid interchange schema for export source object")}, "allowed",
      "permissive");
  if (!expect(validate_export_result_document(export_invalid_schema, error_message),
              "export invalid schema result should validate")) {
    return 1;
  }
  if (!expect(export_invalid_schema.find("\"code\": \"canonfs-export-invalid-schema\"") !=
                  std::string::npos,
              "export invalid schema should emit explicit code")) {
    return 1;
  }

  const std::string export_hash_mismatch = render_export_result(
      "error", {"sha3-256:weird"}, "host-file", "/tmp/out.bin", "", "", {}, {},
      {issue("hash_mismatch", "object hash mismatch: expected sha3-256:weird, got sha3-256:other")},
      "allowed", "permissive");
  if (!expect(validate_export_result_document(export_hash_mismatch, error_message),
              "export hash mismatch result should validate")) {
    return 1;
  }
  if (!expect(export_hash_mismatch.find("\"code\": \"canonfs-export-hash-mismatch\"") !=
                  std::string::npos,
              "export hash mismatch should emit explicit code")) {
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
      "  \"provenance_schema\": \"t81.canonfs-export-provenance.v1\",\n"
      "  \"provenance_ref\": \"sha3-256:prov002\",\n"
      "  \"manifest_schema\": null,\n"
      "  \"warnings\": [],\n"
      "  \"errors\": [{\"kind\": \"wrong-kind\", \"message\": \"bad\", \"code\": \"x\", "
      "\"reason\": \"bad_reason\"}],\n"
      "  \"policy_result\": \"allowed\",\n"
      "  \"policy_profile\": \"permissive\",\n"
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

  const std::string missing_provenance_import_result =
      "{\n"
      "  \"schema\": \"t81.canonfs-import.v1\",\n"
      "  \"status\": \"ok\",\n"
      "  \"source_kind\": \"host-file\",\n"
      "  \"source_ref\": \"/tmp/input.bin\",\n"
      "  \"imported_objects\": [\"sha3-256:abc123\"],\n"
      "  \"manifest_schema\": \"t81.canonfs-interchange-manifest.v1\",\n"
      "  \"manifest_ref\": \"sha3-256:manifest001\",\n"
      "  \"imported_paths\": [\"input.bin\"],\n"
      "  \"warnings\": [],\n"
      "  \"errors\": [],\n"
      "  \"policy_result\": \"allowed\",\n"
      "  \"policy_profile\": \"permissive\",\n"
      "  \"normalization_summary\": {\n"
      "    \"timestamps\": \"provenance-only\",\n"
      "    \"ownership\": \"provenance-only\",\n"
      "    \"mode_hint\": \"preserved\"\n"
      "  }\n"
      "}\n";
  if (!expect(!validate_import_result_document(missing_provenance_import_result, error_message),
              "import result missing provenance should fail validation")) {
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
    if (!expect(import_outcome.policy_profile == "permissive",
                "core import_path policy profile mismatch")) {
      return 1;
    }
    if (!expect(import_outcome.imported_objects.size() == 1,
                "core import_path object count mismatch")) {
      return 1;
    }

    t81::canonfs::ExportOptions export_options;
    export_options.canonfs_root = import_options.canonfs_root;
    const auto export_outcome =
        t81::canonfs::export_ref(import_outcome.imported_objects.front(), restored, export_options);
    if (!expect(export_outcome.ok(), "core export_ref should succeed")) return 1;
    if (!expect(export_outcome.policy_profile == "permissive",
                "core export_ref policy profile mismatch")) {
      return 1;
    }

    std::ifstream in(restored);
    std::string restored_text;
    std::getline(in, restored_text);
    if (!expect(restored_text == "canonfs-interchange-ops", "core export_ref payload mismatch")) {
      return 1;
    }

    export_options.policy_evaluator = [](std::string_view, std::string_view, std::string_view) {
      return t81::canonfs::InterchangePolicyDecision{.allowed = false, .reason = "blocked"};
    };
    const auto denied_export = t81::canonfs::export_ref(import_outcome.imported_objects.front(),
                                                        root / "denied.txt", export_options);
    if (!expect(!denied_export.ok(), "policy-denied core export should fail")) return 1;
    if (!expect(denied_export.policy_result == "denied", "policy-denied export result mismatch")) {
      return 1;
    }
    if (!expect(!denied_export.errors.empty(), "policy-denied export should record an error")) {
      return 1;
    }
    if (!expect(denied_export.errors.front().reason == "policy_denied",
                "policy-denied export should record explicit reason")) {
      return 1;
    }
    if (!expect(denied_export.errors.front().message.find("policy denied export:") == 0,
                "policy-denied export error should identify the export denial")) {
      return 1;
    }
    export_options.policy_evaluator = {};

    t81::canonfs::ImportOptions denied_import_options;
    denied_import_options.canonfs_root = import_options.canonfs_root;
    denied_import_options.policy_profile = t81::canonfs::InterchangePolicyProfile::ExportOnly;
    const auto denied_import = t81::canonfs::import_path(source, denied_import_options);
    if (!expect(!denied_import.ok(), "export-only profile should deny import")) return 1;
    if (!expect(denied_import.policy_result == "denied",
                "export-only profile import result mismatch")) {
      return 1;
    }
    if (!expect(denied_import.policy_profile == "export-only",
                "export-only profile import policy profile mismatch")) {
      return 1;
    }

    const auto missing_import = t81::canonfs::import_path(root / "missing.txt", import_options);
    if (!expect(!missing_import.ok(), "missing input import should fail")) return 1;
    if (!expect(!missing_import.errors.empty(), "missing input import should record an error")) {
      return 1;
    }
    if (!expect(missing_import.errors.front().reason == "missing_source",
                "missing input import should record explicit reason")) {
      return 1;
    }
    if (!expect(missing_import.errors.front().message.find("input path does not exist:") == 0,
                "missing input import error should identify the missing path")) {
      return 1;
    }

    t81::canonfs::ExportOptions import_only_export_options;
    import_only_export_options.canonfs_root = import_options.canonfs_root;
    import_only_export_options.policy_profile = t81::canonfs::InterchangePolicyProfile::ImportOnly;
    const auto import_only_export =
        t81::canonfs::export_ref(import_outcome.imported_objects.front(),
                                 root / "import-only-denied.txt", import_only_export_options);
    if (!expect(!import_only_export.ok(), "import-only profile should deny export")) return 1;
    if (!expect(import_only_export.policy_result == "denied",
                "import-only profile export result mismatch")) {
      return 1;
    }
    if (!expect(import_only_export.policy_profile == "import-only",
                "import-only profile export policy profile mismatch")) {
      return 1;
    }

    const auto missing_export =
        t81::canonfs::export_ref("sha3-256:missing", root / "missing-out.txt", export_options);
    if (!expect(!missing_export.ok(), "missing object export should fail")) return 1;
    if (!expect(!missing_export.errors.empty(), "missing object export should record an error")) {
      return 1;
    }
    if (!expect(missing_export.errors.front().reason == "missing_object",
                "missing object export should record explicit reason")) {
      return 1;
    }
    if (!expect(missing_export.errors.front().message.find("object not found: sha3-256:missing") ==
                    0,
                "missing object export should identify the missing CanonFS object")) {
      return 1;
    }

    const fs::path object_path =
        import_options.canonfs_root / "objects" /
        (import_outcome.imported_objects.front().substr(std::string("sha3-256:").size()) + ".blk");
    const fs::path other_source = root / "other-payload.txt";
    {
      std::ofstream out(other_source);
      out << "canonfs-other-payload";
    }
    const auto other_import = t81::canonfs::import_path(other_source, import_options);
    if (!expect(other_import.ok(), "other CanonFS object import should succeed")) return 1;
    const fs::path other_object_path =
        import_options.canonfs_root / "objects" /
        (other_import.imported_objects.front().substr(std::string("sha3-256:").size()) + ".blk");
    fs::copy_file(other_object_path, object_path, fs::copy_options::overwrite_existing, ec);
    if (!expect(!ec, "tampered object copy should succeed")) return 1;
    const auto tampered_export =
        t81::canonfs::export_ref(import_outcome.imported_objects.front(), root / "tampered.txt",
                                 export_options);
    if (!expect(!tampered_export.ok(), "tampered object export should fail")) return 1;
    if (!expect(!tampered_export.errors.empty(), "tampered export should record an error")) {
      return 1;
    }
    if (!expect(tampered_export.errors.front().reason == "hash_mismatch",
                "tampered export should report hash mismatch")) {
      return 1;
    }

    const fs::path example_root = fs::absolute(fs::path(__FILE__)).parent_path().parent_path().parent_path();
    const fs::path golden_input =
        example_root / "examples" / "storage-and-canonfs" / "canonfs-interchange" / "input";
    const auto golden_import = t81::canonfs::import_path(golden_input, import_options);
    if (!expect(golden_import.ok(), "golden example import should succeed")) return 1;
    if (!expect(!golden_import.manifest_ref.empty(), "golden example should emit manifest")) {
      return 1;
    }
    const fs::path golden_out = root / "golden-export";
    const auto golden_export =
        t81::canonfs::export_ref(golden_import.manifest_ref, golden_out, export_options);
    if (!expect(golden_export.ok(), "golden example export should succeed")) return 1;
    std::ifstream golden_alpha(golden_out / "alpha.txt");
    std::string golden_alpha_text;
    std::getline(golden_alpha, golden_alpha_text);
    if (!expect(golden_alpha_text == "alpha", "golden example alpha payload mismatch")) {
      return 1;
    }

    fs::remove_all(root, ec);
    if (!expect(!ec, "ops test root cleanup failed")) return 1;
  }

  return 0;
}
