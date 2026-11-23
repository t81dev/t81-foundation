#include "tools/cli/core/cmd_axion_canonfs.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "t81/axion/policy_validator.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/canonfs/interchange_ops.hpp"
#include "t81/canonfs/performance_cli.hpp"
#include "t81/canonfs/storage_key.hpp"
#include "t81/cli/driver.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/vm.hpp"
#include "tools/cli/core/help_text.hpp"
#include "tools/cli/logging.hpp"

namespace fs = std::filesystem;

std::string json_escape(std::string_view text);
fs::path discover_canonfs_root();

bool is_valid_snapshot_hash_text(std::string_view text) {
  if (text.empty()) {
    return false;
  }
  try {
    (void)t81::hash::CanonHash81::from_string(std::string(text));
    return true;
  } catch (...) {
    return false;
  }
}

int run_axion(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_axion(); });
  }

  const std::string sub = args.command_args[0];
  bool json_out = false;
  int target_tier = 1;
  bool tier_provided = false;
  bool tier_invalid = false;
  std::size_t log_tail_n = 10;
  std::string rollback_to;
  std::string audit_from;
  std::string audit_to;
  std::string simulate_file;
  std::string explain_file;
  std::vector<std::string> snapshot_diff_hashes;
  const fs::path canonfs_root = discover_canonfs_root();

  auto count_snapshot_dirs = [&](const fs::path& root) {
    std::size_t count = 0;
    std::error_code ec;
    const fs::path snapshots_root = root / "snapshots";
    if (!fs::exists(snapshots_root, ec)) {
      return count;
    }
    for (const auto& entry : fs::directory_iterator(snapshots_root, ec)) {
      if (ec) break;
      if (entry.is_directory()) {
        ++count;
      }
    }
    return count;
  };

  auto count_canonfs_objects = [&](const fs::path& root) {
    std::size_t count = 0;
    std::error_code ec;
    const fs::path objects_root = root / "objects";
    if (!fs::exists(objects_root, ec)) {
      return count;
    }
    for (const auto& entry : fs::directory_iterator(objects_root, ec)) {
      if (ec) break;
      if (entry.is_regular_file() && entry.path().extension() == ".blk") {
        ++count;
      }
    }
    return count;
  };

  auto snapshot_dir_path = [&](const fs::path& root,
                               std::string_view canonical_hash) -> std::optional<fs::path> {
    try {
      return root / "snapshots" / t81::canonfs::snapshot_storage_key_v1(canonical_hash);
    } catch (...) {
      return std::nullopt;
    }
  };

  auto sorted_snapshot_hashes = [&](const fs::path& root) {
    std::vector<std::string> hashes;
    std::error_code ec;
    const fs::path snapshots_root = root / "snapshots";
    if (!fs::exists(snapshots_root, ec)) {
      return hashes;
    }
    for (const auto& entry : fs::directory_iterator(snapshots_root, ec)) {
      if (ec || !entry.is_directory()) {
        continue;
      }
      const auto parsed = t81::canonfs::parse_storage_key_v1(entry.path().filename().string());
      if (parsed) {
        hashes.push_back(parsed->h.to_string());
      }
    }
    std::sort(hashes.begin(), hashes.end(), std::greater<std::string>{});
    return hashes;
  };

  auto latest_snapshot_hash = [&](const fs::path& root) -> std::optional<std::string> {
    const auto hashes = sorted_snapshot_hashes(root);
    if (hashes.empty()) {
      return std::nullopt;
    }
    return hashes.front();
  };

  auto recent_snapshot_hashes = [&](const fs::path& root, std::size_t limit) {
    auto hashes = sorted_snapshot_hashes(root);
    if (hashes.size() > limit) {
      hashes.resize(limit);
    }
    return hashes;
  };

  auto axion_state_path = [&](const fs::path& root) { return root / "axion" / "state.json"; };
  auto json_nullable_axion = [&](const std::optional<std::string>& value) {
    return value ? ("\"" + json_escape(*value) + "\"") : std::string("null");
  };

  struct AxionStateSnapshot {
    bool present = false;
    int tier = 1;
    std::optional<std::string> active_snapshot;
  };

  auto read_axion_state = [&](const fs::path& root) {
    AxionStateSnapshot snapshot;
    std::ifstream state_in(axion_state_path(root));
    if (!state_in) {
      return snapshot;
    }
    snapshot.present = true;
    const std::string state_text((std::istreambuf_iterator<char>(state_in)),
                                 std::istreambuf_iterator<char>());
    std::smatch match;
    if (std::regex_search(state_text, match, std::regex(R"("tier"\s*:\s*([0-9]+))")) &&
        match.size() == 2) {
      snapshot.tier = std::stoi(match[1].str());
    }
    if (std::regex_search(state_text, match,
                          std::regex("\"active_snapshot\"\\s*:\\s*\"([^\"]+)\"")) &&
        match.size() == 2) {
      snapshot.active_snapshot = match[1].str();
    }
    return snapshot;
  };

  auto read_manifest_lines = [&](const std::string& hash) {
    std::vector<std::string> lines;
    if (hash.empty()) {
      return lines;
    }
    const auto dir = snapshot_dir_path(canonfs_root, hash);
    if (!dir) {
      return lines;
    }
    const fs::path manifest = *dir / "MANIFEST.txt";
    std::ifstream in(manifest, std::ios::binary);
    if (!in) {
      return lines;
    }
    std::string line;
    while (std::getline(in, line)) {
      if (!line.empty()) {
        lines.push_back(line);
      }
    }
    std::sort(lines.begin(), lines.end());
    return lines;
  };

  auto write_axion_state = [&](int tier) -> bool {
    std::error_code ec;
    fs::create_directories(axion_state_path(canonfs_root).parent_path(), ec);
    if (ec) {
      error("axion optimize: could not create state directory under " + canonfs_root.string());
      return false;
    }
    std::ofstream out(axion_state_path(canonfs_root), std::ios::binary | std::ios::trunc);
    if (!out) {
      error("axion optimize: could not write axion state file.");
      return false;
    }
    out << "{\n"
        << "  \"schema\": \"t81.axion-state.v1\",\n"
        << "  \"tier\": " << tier << "\n"
        << "}\n";
    return static_cast<bool>(out);
  };

  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const auto& tok = args.command_args[i];
    if (tok == "--json") {
      json_out = true;
    } else if (tok == "--tier" && i + 1 < args.command_args.size()) {
      tier_provided = true;
      try {
        target_tier = std::stoi(args.command_args[++i]);
        if (target_tier < 1 || target_tier > 9) {
          tier_invalid = true;
        }
      } catch (...) {
        tier_invalid = true;
      }
    } else if (tok == "--to" && i + 1 < args.command_args.size()) {
      if (sub == "rollback") {
        rollback_to = args.command_args[++i];
      } else if (sub == "audit") {
        audit_to = args.command_args[++i];
      } else {
        error("axion: unknown argument '" + tok + "'. Run 't81 help axion'.");
        return 1;
      }
    } else if (tok == "--from" && i + 1 < args.command_args.size()) {
      audit_from = args.command_args[++i];
    } else if (tok == "--tail" && i + 1 < args.command_args.size() && sub == "log") {
      try {
        log_tail_n = static_cast<std::size_t>(std::stoull(args.command_args[++i]));
      } catch (...) {
        error("axion log: invalid --tail value.");
        return 1;
      }
    } else if (explain_file.empty() && sub == "explain") {
      explain_file = tok;
    } else if (simulate_file.empty() && sub == "simulate") {
      simulate_file = tok;
    } else if (sub == "snapshot-diff" && snapshot_diff_hashes.size() < 2) {
      snapshot_diff_hashes.push_back(tok);
    } else {
      error("axion: unknown argument '" + tok + "'. Run 't81 help axion'.");
      return 1;
    }
  }

  if (tier_provided && tier_invalid) {
    error("axion: --tier must be an integer in the range 1..9.");
    return 1;
  }
  if (!rollback_to.empty() && !is_valid_snapshot_hash_text(rollback_to)) {
    error("axion rollback: invalid snapshot hash.");
    return 1;
  }
  if (!audit_from.empty() && !is_valid_snapshot_hash_text(audit_from)) {
    error("axion audit: invalid --from snapshot hash.");
    return 1;
  }
  if (!audit_to.empty() && !is_valid_snapshot_hash_text(audit_to)) {
    error("axion audit: invalid --to snapshot hash.");
    return 1;
  }
  for (const auto& hash : snapshot_diff_hashes) {
    if (!is_valid_snapshot_hash_text(hash)) {
      error("axion snapshot-diff: invalid snapshot hash.");
      return 1;
    }
  }

  if (sub == "status") {
    const auto snapshot_hash = latest_snapshot_hash(canonfs_root);
    const auto snapshot_count = count_snapshot_dirs(canonfs_root);
    const auto object_count = count_canonfs_objects(canonfs_root);
    const bool canonfs_ready = fs::exists(canonfs_root);
    auto state = read_axion_state(canonfs_root);
    if (!state.present && tier_provided) {
      state.tier = target_tier;
    }
    const auto active_dir = state.active_snapshot
                                ? snapshot_dir_path(canonfs_root, *state.active_snapshot)
                                : std::nullopt;
    const bool state_snapshot_present = active_dir && fs::exists(*active_dir / "MANIFEST.txt");
    std::vector<std::string> issues;
    if (!canonfs_ready) {
      issues.emplace_back("canonfs_root_missing");
    }
    if (!state.present) {
      issues.emplace_back("axion_state_missing");
    }
    if (state.active_snapshot && !state_snapshot_present) {
      issues.emplace_back("active_snapshot_missing");
    }
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-status.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"axion\": \"active\",\n"
                << "  \"canonfs_ready\": " << (canonfs_ready ? "true" : "false") << ",\n"
                << "  \"state_present\": " << (state.present ? "true" : "false") << ",\n"
                << "  \"object_count\": " << object_count << ",\n"
                << "  \"snapshot_count\": " << snapshot_count << ",\n"
                << "  \"active_snapshot\": ";
      if (snapshot_hash) {
        std::cout << "\"" << json_escape(*snapshot_hash) << "\",\n";
      } else {
        std::cout << "null,\n";
      }
      std::cout << "  \"state_snapshot\": " << json_nullable_axion(state.active_snapshot) << ",\n"
                << "  \"state_snapshot_present\": " << (state_snapshot_present ? "true" : "false")
                << ",\n"
                << "  \"state_updated\": null,\n"
                << "  \"tier\": " << state.tier << ",\n"
                << "  \"issues\": [\n";
      for (std::size_t i = 0; i < issues.size(); ++i) {
        std::cout << "    \"" << json_escape(issues[i]) << "\"";
        if (i + 1 < issues.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "Axion Governor: active\n"
                << "CanonFS root: " << canonfs_root.string() << "\n"
                << "CanonFS ready: " << (canonfs_ready ? "yes" : "no") << "\n"
                << "State file: " << (state.present ? "present" : "missing") << "\n"
                << "Stored objects: " << object_count << "\n"
                << "Snapshots: " << snapshot_count << "\n"
                << "Active snapshot: " << (snapshot_hash ? *snapshot_hash : "<none>") << "\n"
                << "State snapshot: " << (state.active_snapshot ? *state.active_snapshot : "<none>")
                << "\n"
                << "State snapshot present: " << (state_snapshot_present ? "yes" : "no") << "\n"
                << "Active tier: " << state.tier << "\n";
      if (!issues.empty()) {
        std::cout << "Issues:\n";
        for (const auto& issue : issues) {
          std::cout << "  " << issue << "\n";
        }
      }
    }
    return 0;
  }

  if (sub == "optimize") {
    const auto previous_state = read_axion_state(canonfs_root);
    const auto previous_snapshot = previous_state.active_snapshot
                                       ? previous_state.active_snapshot
                                       : latest_snapshot_hash(canonfs_root);
    if (!write_axion_state(target_tier)) {
      return 1;
    }
    std::string snapshot_error;
    const auto snapshot_hash =
        t81::cli::canonfs_capture_snapshot_hash(canonfs_root, &snapshot_error);
    if (!snapshot_hash) {
      error("axion optimize: failed to capture current CanonFS snapshot: " + snapshot_error);
      return 1;
    }
    std::vector<std::string> only_previous;
    std::vector<std::string> only_current;
    if (previous_snapshot && *previous_snapshot != *snapshot_hash) {
      const auto previous_lines = read_manifest_lines(*previous_snapshot);
      const auto current_lines = read_manifest_lines(*snapshot_hash);
      std::set_difference(previous_lines.begin(), previous_lines.end(), current_lines.begin(),
                          current_lines.end(), std::back_inserter(only_previous));
      std::set_difference(current_lines.begin(), current_lines.end(), previous_lines.begin(),
                          previous_lines.end(), std::back_inserter(only_current));
    }
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-optimize.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"status\": \"ok\",\n"
                << "  \"action\": \"optimize\",\n"
                << "  \"tier\": " << target_tier << ",\n"
                << "  \"previous_snapshot\": " << json_nullable_axion(previous_snapshot) << ",\n"
                << "  \"active_snapshot\": ";
      if (snapshot_hash) {
        std::cout << "\"" << json_escape(*snapshot_hash) << "\",\n";
      } else {
        std::cout << "null,\n";
      }
      std::cout << "  \"only_previous_count\": " << only_previous.size() << ",\n"
                << "  \"only_current_count\": " << only_current.size() << "\n"
                << "}\n";
    } else {
      info("Axion optimize: captured CanonFS snapshot, diffed it against the previous state, and "
           "recorded requested tier " +
           std::to_string(target_tier) + ".");
      std::cout << "Optimization receipt recorded.\n";
      std::cout << "Tier: " << target_tier << "\n";
      std::cout << "Previous snapshot: " << (previous_snapshot ? *previous_snapshot : "<none>")
                << "\n";
      std::cout << "Snapshot: " << (snapshot_hash ? *snapshot_hash : "<none>") << "\n";
      std::cout << "Only previous: " << only_previous.size() << "\n";
      std::cout << "Only current:  " << only_current.size() << "\n";
    }
    return 0;
  }

  if (sub == "simulate") {
    if (simulate_file.empty()) {
      error("axion simulate requires a .tisc file argument.");
      return 1;
    }
    if (!fs::exists(simulate_file)) {
      error("axion simulate: file not found: " + simulate_file);
      return 1;
    }
    if (fs::path(simulate_file).extension() != ".tisc") {
      error("axion simulate requires a .tisc input file.");
      return 1;
    }
    auto program = t81::tisc::load_program(simulate_file);
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    const bool ok = static_cast<bool>(result);
    if (json_out) {
      std::cout << "{" << "\"schema\":\"t81.axion-simulate.v1\","
                << "\"ok\":" << (ok ? "true" : "false") << "," << "\"status\":\""
                << (ok ? "ok" : "trap") << "\"," << "\"action\":\"simulate\"," << "\"program\":\""
                << json_escape(simulate_file) << "\","
                << "\"trace_entries\":" << vm->state().trace.size() << ","
                << "\"axion_events\":" << vm->state().axion_log.size();
      if (ok) {
        std::cout << ",\"trap\":null";
      } else {
        std::cout << ",\"trap\":\"" << t81::vm::to_string(result.error()) << "\"";
      }
      std::cout << "}\n";
    } else if (ok) {
      info("Axion simulate: executed '" + simulate_file + "' under current policy state.");
      std::cout << "Simulation complete. Trace entries=" << vm->state().trace.size()
                << " axion_events=" << vm->state().axion_log.size() << "\n";
    } else {
      error("Axion simulation trapped: " + std::string(t81::vm::to_string(result.error())));
      return 1;
    }
    return 0;
  }

  if (sub == "explain") {
    if (explain_file.empty()) {
      error("axion explain requires a policy file argument.");
      return 1;
    }
    std::ifstream ifs(explain_file, fs::path(explain_file).extension() == ".axionb"
                                        ? std::ios::binary
                                        : std::ios::in);
    if (!ifs) {
      error("axion explain: could not open file: " + explain_file);
      return 1;
    }
    t81::axion::Policy policy;
    std::optional<t81::axion::PolicyViolation> violation;
    if (fs::path(explain_file).extension() == ".axionb") {
      auto res = t81::axion::Policy::deserialize(ifs);
      if (!res) {
        error("axion explain: policy deserialization error: " + res.error());
        return 1;
      }
      policy = std::move(res.value());
    } else {
      std::string policy_text((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());
      auto parsed = t81::axion::parse_policy(policy_text);
      if (!parsed) {
        error("axion explain: policy parse error: " + parsed.error());
        return 1;
      }
      policy = std::move(parsed.value());
      violation = t81::axion::PolicyValidator::validate_policy(policy_text);
    }
    const bool ok = !violation || !violation->is_violation;
    if (json_out) {
      std::cout << "{" << "\"schema\":\"t81.axion-explain.v1\","
                << "\"ok\":" << (ok ? "true" : "false") << "," << "\"policy\":\""
                << json_escape(explain_file) << "\"," << "\"tier\":" << policy.tier << ","
                << "\"allowed_tensor_hashes\":" << policy.allowed_tensor_hashes.size() << ","
                << "\"loop_hints\":" << policy.loops.size() << ","
                << "\"match_guards\":" << policy.match_guards.size();
      if (violation) {
        std::cout << ",\"rule\":\"" << json_escape(violation->rule) << "\"," << "\"reason\":\""
                  << json_escape(violation->reason) << "\"";
      }
      std::cout << "}\n";
    } else {
      std::cout << "Policy:                " << explain_file << "\n";
      std::cout << "Tier:                  " << policy.tier << "\n";
      std::cout << "Allowed tensor hashes: " << policy.allowed_tensor_hashes.size() << "\n";
      std::cout << "Loop hints:            " << policy.loops.size() << "\n";
      std::cout << "Match guards:          " << policy.match_guards.size() << "\n";
      if (violation) {
        std::cout << "Validation:            " << (violation->is_violation ? "VIOLATION" : "VALID")
                  << "\n";
        std::cout << "Rule:                  " << violation->rule << "\n";
        std::cout << "Reason:                " << violation->reason << "\n";
      }
    }
    return ok ? 0 : 1;
  }

  if (sub == "snapshot") {
    return t81::cli::canonfs_snapshot(canonfs_root, json_out);
  }

  if (sub == "snapshot-diff") {
    if (snapshot_diff_hashes.size() != 2) {
      error("axion snapshot-diff requires two snapshot hashes.");
      return 1;
    }
    return t81::cli::canonfs_snapshot_diff(snapshot_diff_hashes[0], snapshot_diff_hashes[1],
                                           canonfs_root, json_out);
  }

  if (sub == "rollback") {
    if (rollback_to.empty()) {
      error("axion rollback requires --to <hash>.");
      return 1;
    }
    return t81::cli::canonfs_rollback(rollback_to, canonfs_root, json_out);
  }

  if (sub == "log") {
    const fs::path state_path = canonfs_root / "axion" / "state.json";
    const auto state = read_axion_state(canonfs_root);
    const std::size_t snap_count = count_snapshot_dirs(canonfs_root);
    const std::size_t obj_count = count_canonfs_objects(canonfs_root);
    const auto recent = recent_snapshot_hashes(canonfs_root, log_tail_n);
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-log.v1\",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"state_present\": " << (state.present ? "true" : "false") << ",\n"
                << "  \"tier\": " << state.tier << ",\n"
                << "  \"active_snapshot\": " << json_nullable_axion(state.active_snapshot) << ",\n"
                << "  \"snapshots\": " << snap_count << ",\n"
                << "  \"objects\": " << obj_count << ",\n"
                << "  \"tail\": " << log_tail_n << ",\n"
                << "  \"receipt_persistence\": \"not_persisted\",\n"
                << "  \"recent_snapshots\": [\n";
      for (std::size_t i = 0; i < recent.size(); ++i) {
        std::cout << "    \"" << json_escape(recent[i]) << "\"";
        if (i + 1 < recent.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n"
                << "}\n";
    } else {
      std::cout << "CanonFS root:  " << canonfs_root.string() << "\n";
      std::cout << "State file:    " << (state.present ? "present" : "missing") << "\n";
      std::cout << "Tier:          " << state.tier << "\n";
      std::cout << "State snap:    " << (state.active_snapshot ? *state.active_snapshot : "<none>")
                << "\n";
      std::cout << "Snapshots:     " << snap_count << "\n";
      std::cout << "Objects:       " << obj_count << "\n";
      std::cout << "Receipts:      not persisted in current release\n";
      std::cout << "Recent snaps:  " << recent.size() << " (tail=" << log_tail_n << ")\n";
      for (const auto& hash : recent) {
        std::cout << "  " << hash << "\n";
      }
      if (state.present) {
        std::ifstream sf(state_path);
        if (sf) {
          std::cout << "\nAxion state:\n";
          std::string sl;
          while (std::getline(sf, sl)) std::cout << "  " << sl << "\n";
        }
      }
    }
    return 0;
  }

  if (sub == "audit") {
    const auto latest = latest_snapshot_hash(canonfs_root);
    const auto state = read_axion_state(canonfs_root);
    const std::string from_hash =
        audit_from.empty()
            ? (state.active_snapshot ? *state.active_snapshot : (latest ? *latest : ""))
            : audit_from;
    const std::string to_hash = audit_to.empty() ? (latest ? *latest : "") : audit_to;
    const std::size_t snap_count = count_snapshot_dirs(canonfs_root);
    const std::size_t obj_count = count_canonfs_objects(canonfs_root);
    const bool state_ok = state.present;
    std::vector<std::string> only_from;
    std::vector<std::string> only_to;
    if (!from_hash.empty() && !to_hash.empty()) {
      const auto from_lines = read_manifest_lines(from_hash);
      const auto to_lines = read_manifest_lines(to_hash);
      std::set_difference(from_lines.begin(), from_lines.end(), to_lines.begin(), to_lines.end(),
                          std::back_inserter(only_from));
      std::set_difference(to_lines.begin(), to_lines.end(), from_lines.begin(), from_lines.end(),
                          std::back_inserter(only_to));
    }
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-audit.v1\",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"state_present\": " << (state_ok ? "true" : "false") << ",\n"
                << "  \"snapshots\": " << snap_count << ",\n"
                << "  \"objects\": " << obj_count << ",\n"
                << "  \"tier\": " << state.tier << ",\n"
                << "  \"receipt_persistence\": \"not_persisted\",\n"
                << "  \"from\": "
                << (from_hash.empty() ? "null" : "\"" + json_escape(from_hash) + "\"") << ",\n"
                << "  \"to\": " << (to_hash.empty() ? "null" : "\"" + json_escape(to_hash) + "\"")
                << ",\n"
                << "  \"only_from_count\": " << only_from.size() << ",\n"
                << "  \"only_to_count\": " << only_to.size() << "\n"
                << "}\n";
      return 0;
    }
    std::cout << "CanonFS root:  " << canonfs_root.string() << "\n";
    std::cout << "State file:    " << (state_ok ? "present" : "missing") << "\n";
    std::cout << "Tier:          " << state.tier << "\n";
    std::cout << "Snapshots:     " << snap_count << "\n";
    std::cout << "Objects:       " << obj_count << "\n";
    std::cout << "Receipts:      not persisted in current release\n";
    std::cout << "From snapshot: " << (from_hash.empty() ? "<none>" : from_hash) << "\n";
    std::cout << "To snapshot:   " << (to_hash.empty() ? "<none>" : to_hash) << "\n";
    std::cout << "Only from:     " << only_from.size() << "\n";
    std::cout << "Only to:       " << only_to.size() << "\n";
    return 0;
  }

  error("axion: unknown subcommand '" + sub + "'. Run 't81 axion --help' for usage.");
  return 1;
}

int run_canonfs_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_canonfs(); });
  }

  const std::string action = args.command_args[0];
  fs::path canonfs_root = discover_canonfs_root();
  bool as_json = false;
  bool dry_run = false;
  bool verify_hashes = false;
  std::optional<fs::path> output_path = args.output;
  std::optional<fs::path> policy_path = args.policy;
  std::optional<std::string> policy_profile;
  std::vector<std::string> positional;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--canonfs-root") {
      if (++i >= args.command_args.size()) {
        error("canonfs: missing value for --canonfs-root.");
        return 1;
      }
      canonfs_root = args.command_args[i];
    } else if (token == "--json") {
      as_json = true;
    } else if (token == "--policy") {
      if (++i >= args.command_args.size()) {
        error("canonfs: missing value for --policy.");
        return 1;
      }
      policy_path = fs::path(args.command_args[i]);
    } else if (token == "--policy-profile") {
      if (++i >= args.command_args.size()) {
        error("canonfs: missing value for --policy-profile.");
        return 1;
      }
      policy_profile = args.command_args[i];
    } else if (token == "--dry-run") {
      dry_run = true;
    } else if (token == "--verify-hashes") {
      verify_hashes = true;
    } else if (token == "--out" || token == "--output" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("canonfs get: missing value for " + token + ".");
        return 1;
      }
      output_path = fs::path(args.command_args[i]);
    } else if (token == "--to" && action == "rollback") {
      if (++i >= args.command_args.size()) {
        error("canonfs rollback: missing value for --to.");
        return 1;
      }
      positional.push_back(args.command_args[i]);
    } else if (action == "snapshot-diff" && positional.size() < 2) {
      positional.push_back(token);
    } else if (!token.empty() && token[0] == '-') {
      error("canonfs: unknown option '" + token + "'. Run 't81 help canonfs'.");
      return 1;
    } else {
      positional.push_back(token);
    }
  }

  if (action == "put-file") {
    if (positional.size() != 1) {
      error("canonfs put-file requires exactly one input file.");
      return 1;
    }
    return t81::cli::canonfs_put_file(positional[0], canonfs_root);
  }
  if (action == "import") {
    if (positional.size() != 1) {
      error("canonfs import requires exactly one input path.");
      return 1;
    }
    return t81::cli::canonfs_import(positional[0], canonfs_root, policy_path, policy_profile,
                                    as_json);
  }
  if (action == "put-tensor") {
    if (positional.size() != 1) {
      error("canonfs put-tensor requires exactly one input file.");
      return 1;
    }
    return t81::cli::canonize_tensor(positional[0], canonfs_root);
  }
  if (action == "ls") {
    if (!positional.empty()) {
      error("canonfs ls does not accept positional arguments.");
      return 1;
    }
    return t81::cli::canonfs_list(canonfs_root, as_json);
  }
  if (action == "get") {
    if (positional.size() != 1) {
      error("canonfs get requires exactly one canonical hash.");
      return 1;
    }
    return t81::cli::canonfs_get(positional[0], output_path, canonfs_root, as_json);
  }
  if (action == "export") {
    if (positional.size() != 1) {
      error("canonfs export requires exactly one canonical hash.");
      return 1;
    }
    if (!output_path) {
      error("canonfs export requires -o/--out <path>.");
      return 1;
    }
    return t81::cli::canonfs_export(positional[0], *output_path, canonfs_root, policy_path,
                                    policy_profile, as_json);
  }
  if (action == "stat") {
    if (positional.size() != 1) {
      error("canonfs stat requires exactly one canonical hash.");
      return 1;
    }
    return t81::cli::canonfs_stat(positional[0], canonfs_root, as_json);
  }
  if (action == "verify") {
    if (positional.size() != 1) {
      error("canonfs verify requires exactly one canonical hash.");
      return 1;
    }
    return t81::cli::canonfs_verify(positional[0], canonfs_root, as_json);
  }
  if (action == "snapshot") {
    if (!positional.empty()) {
      error("canonfs snapshot does not accept positional arguments.");
      return 1;
    }
    return t81::cli::canonfs_snapshot(canonfs_root, as_json);
  }
  if (action == "snapshot-diff") {
    if (positional.size() != 2) {
      error("canonfs snapshot-diff requires exactly two snapshot hashes.");
      return 1;
    }
    return t81::cli::canonfs_snapshot_diff(positional[0], positional[1], canonfs_root, as_json);
  }
  if (action == "rollback") {
    if (positional.size() != 1) {
      error("canonfs rollback requires --to <hash>.");
      return 1;
    }
    if (dry_run) {
      std::string hash_err;
      if (!is_valid_snapshot_hash_text(positional[0])) {
        error("canonfs rollback: invalid snapshot hash.");
        return 1;
      }
      fs::path snapshot_dir;
      try {
        snapshot_dir =
            canonfs_root / "snapshots" / t81::canonfs::snapshot_storage_key_v1(positional[0]);
      } catch (...) {
        error("canonfs rollback: invalid snapshot hash.");
        return 1;
      }
      const bool exists = fs::exists(snapshot_dir);
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.canonfs-rollback.v1\",\n"
                  << "  \"ok\": " << (exists ? "true" : "false") << ",\n"
                  << "  \"dry_run\": true,\n"
                  << "  \"snapshot\": \"" << json_escape(positional[0]) << "\",\n"
                  << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\"\n"
                  << "}\n";
      } else {
        std::cout << "Rollback dry-run\n";
        std::cout << "Snapshot:     " << positional[0] << "\n";
        std::cout << "CanonFS root: " << canonfs_root.string() << "\n";
        std::cout << "Status:       " << (exists ? "ready" : "snapshot not found") << "\n";
      }
      return exists ? 0 : 1;
    }
    return t81::cli::canonfs_rollback(positional[0], canonfs_root, as_json);
  }

  if (action == "gc") {
    // Collect all objects
    const fs::path objects_dir = canonfs_root / "objects";
    // Collect hashes referenced by any snapshot
    std::unordered_set<std::string> referenced;
    std::error_code ec;
    const fs::path snaps_dir = canonfs_root / "snapshots";
    if (fs::exists(snaps_dir)) {
      for (const auto& snap_entry : fs::directory_iterator(snaps_dir, ec)) {
        if (ec) break;
        if (!snap_entry.is_directory()) continue;
        for (const auto& f : fs::directory_iterator(snap_entry.path(), ec)) {
          if (ec) break;
          if (f.is_regular_file()) referenced.insert(f.path().stem().string());
        }
      }
    }
    std::size_t removed = 0;
    std::uintmax_t bytes_freed = 0;
    std::vector<std::string> removable;
    if (fs::exists(objects_dir)) {
      for (const auto& obj : fs::directory_iterator(objects_dir, ec)) {
        if (ec) break;
        if (!obj.is_regular_file()) continue;
        const std::string stem = obj.path().stem().string();
        if (referenced.find(stem) == referenced.end()) {
          const std::uintmax_t sz = fs::file_size(obj.path(), ec);
          if (dry_run) {
            removable.push_back(stem);
            ++removed;
            bytes_freed += sz;
          } else {
            fs::remove(obj.path(), ec);
            if (!ec) {
              ++removed;
              bytes_freed += sz;
            }
          }
        }
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-gc.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"dry_run\": " << (dry_run ? "true" : "false") << ",\n"
                << "  \"removed\": " << removed << ",\n"
                << "  \"bytes_freed\": " << bytes_freed;
      if (dry_run) {
        std::cout << ",\n  \"objects\": [\n";
        for (std::size_t i = 0; i < removable.size(); ++i) {
          std::cout << "    \"" << json_escape(removable[i]) << "\"";
          if (i + 1 < removable.size()) {
            std::cout << ",";
          }
          std::cout << "\n";
        }
        std::cout << "  ]\n";
      } else {
        std::cout << "\n";
      }
      std::cout << "}\n";
    } else {
      std::cout << "GC" << (dry_run ? " dry-run" : "") << ": "
                << (dry_run ? "would remove " : "removed ") << removed << " object(s), "
                << (dry_run ? "would free " : "freed ") << bytes_freed << " byte(s)\n";
    }
    return 0;
  }

  if (action == "fsck") {
    const fs::path objects_dir = canonfs_root / "objects";
    const fs::path snapshots_dir = canonfs_root / "snapshots";
    std::size_t checked_objects = 0;
    std::size_t checked_snapshots = 0;
    std::vector<std::string> errors;
    std::error_code ec;

    if (fs::exists(objects_dir)) {
      for (const auto& obj : fs::directory_iterator(objects_dir, ec)) {
        if (ec) {
          errors.push_back("failed reading objects directory");
          break;
        }
        if (fs::is_symlink(obj.symlink_status())) {
          errors.push_back(obj.path().filename().string() +
                           ": symlinked objects are not permitted");
          continue;
        }
        if (!obj.is_regular_file() || obj.path().extension() != ".blk") {
          continue;
        }
        ++checked_objects;
        std::ifstream in(obj.path(), std::ios::binary);
        if (!in) {
          errors.push_back(obj.path().filename().string() + ": object unreadable");
          continue;
        }
        if (verify_hashes) {
          const std::string stem = obj.path().stem().string();
          const auto expected = t81::canonfs::parse_storage_key_v1(stem);
          if (!expected.has_value()) {
            errors.push_back(obj.path().filename().string() + ": invalid storage key");
            continue;
          }
          std::vector<std::byte> bytes;
          in.seekg(0, std::ios::end);
          const auto sz = in.tellg();
          if (sz > 0) {
            bytes.resize(static_cast<std::size_t>(sz));
            in.seekg(0, std::ios::beg);
            in.read(reinterpret_cast<char*>(bytes.data()), sz);
          }
          if (in.bad()) {
            errors.push_back(obj.path().filename().string() + ": object read failed");
            continue;
          }
          const auto actual = t81::hash::hash_bytes(bytes);
          if (actual != expected->h) {
            errors.push_back(obj.path().filename().string() + ": hash mismatch (expected " + stem +
                             ", actual " +
                             t81::canonfs::storage_key_v1(t81::canonfs::CanonHash{actual}) + ")");
          }
        } else {
          in.peek();
          if (in.bad()) {
            errors.push_back(obj.path().filename().string() + ": object read failed");
          }
        }
      }
    }

    if (fs::exists(snapshots_dir)) {
      for (const auto& snap : fs::directory_iterator(snapshots_dir, ec)) {
        if (ec) {
          errors.push_back("failed reading snapshots directory");
          break;
        }
        if (fs::is_symlink(snap.symlink_status())) {
          errors.push_back("snapshot entry " + snap.path().filename().string() +
                           ": symlinked snapshots are not permitted");
          continue;
        }
        if (!snap.is_directory()) {
          continue;
        }
        ++checked_snapshots;
        const fs::path manifest = snap.path() / "MANIFEST.txt";
        if (!fs::exists(manifest)) {
          errors.push_back("snapshot " + snap.path().filename().string() +
                           ": missing MANIFEST.txt");
          continue;
        }
        for (auto it = fs::recursive_directory_iterator(snap.path(), ec);
             it != fs::recursive_directory_iterator(); ++it) {
          if (ec) {
            errors.push_back("snapshot " + snap.path().filename().string() + ": scan failed");
            break;
          }
          if (fs::is_symlink(it->symlink_status())) {
            errors.push_back("snapshot " + snap.path().filename().string() +
                             ": symlinked content is not permitted");
            break;
          }
        }
        std::ifstream in(manifest, std::ios::binary);
        if (!in) {
          errors.push_back("snapshot " + snap.path().filename().string() + ": manifest unreadable");
        }
      }
    }

    const bool ok = errors.empty();
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-fsck.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"checked_objects\": " << checked_objects << ",\n"
                << "  \"checked_snapshots\": " << checked_snapshots << ",\n"
                << "  \"errors\": [\n";
      for (std::size_t i = 0; i < errors.size(); ++i) {
        std::cout << "    \"" << json_escape(errors[i]) << "\"";
        if (i + 1 < errors.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "CanonFS fsck\n";
      std::cout << "Root:       " << canonfs_root.string() << "\n";
      std::cout << "Objects:    " << checked_objects << "\n";
      std::cout << "Snapshots:  " << checked_snapshots << "\n";
      std::cout << "Status:     " << (ok ? "ok" : "errors") << "\n";
      for (const auto& msg : errors) {
        std::cout << "  " << msg << "\n";
      }
    }
    return ok ? 0 : 1;
  }

  if (action == "repair") {
    const fs::path snapshots_dir = canonfs_root / "snapshots";
    std::size_t repaired_snapshots = 0;
    std::uintmax_t bytes_reclaimed = 0;
    std::vector<std::string> repaired_paths;
    std::error_code ec;

    if (fs::exists(snapshots_dir)) {
      for (const auto& snap : fs::directory_iterator(snapshots_dir, ec)) {
        if (ec) {
          error("canonfs repair: failed reading snapshots directory.");
          return 1;
        }
        if (!snap.is_directory()) {
          continue;
        }
        const fs::path legacy_objects = snap.path() / "objects";
        if (!fs::exists(legacy_objects)) {
          continue;
        }
        if (fs::is_symlink(fs::symlink_status(legacy_objects, ec))) {
          error("canonfs repair: refusing to operate on symlinked path " + legacy_objects.string());
          return 1;
        }
        std::uintmax_t dir_bytes = 0;
        for (auto it = fs::recursive_directory_iterator(legacy_objects, ec);
             it != fs::recursive_directory_iterator(); ++it) {
          if (ec) break;
          if (it->is_regular_file()) {
            dir_bytes += fs::file_size(it->path(), ec);
            if (ec) break;
          }
        }
        if (ec) {
          error("canonfs repair: failed scanning " + legacy_objects.string());
          return 1;
        }
        repaired_paths.push_back(fs::relative(legacy_objects, canonfs_root).string());
        ++repaired_snapshots;
        bytes_reclaimed += dir_bytes;
        if (!dry_run) {
          fs::remove_all(legacy_objects, ec);
          if (ec) {
            error("canonfs repair: failed removing " + legacy_objects.string());
            return 1;
          }
        }
      }
    }

    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-repair.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"dry_run\": " << (dry_run ? "true" : "false") << ",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"repaired_snapshots\": " << repaired_snapshots << ",\n"
                << "  \"bytes_reclaimed\": " << bytes_reclaimed << ",\n"
                << "  \"paths\": [\n";
      for (std::size_t i = 0; i < repaired_paths.size(); ++i) {
        std::cout << "    \"" << json_escape(repaired_paths[i]) << "\"";
        if (i + 1 < repaired_paths.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "CanonFS repair" << (dry_run ? " dry-run" : "") << "\n";
      std::cout << "Root:              " << canonfs_root.string() << "\n";
      std::cout << "Repaired snapshots:" << repaired_snapshots << "\n";
      std::cout << "Bytes reclaimed:   " << bytes_reclaimed << "\n";
      for (const auto& path : repaired_paths) {
        std::cout << "  " << path << "\n";
      }
    }
    return 0;
  }

  error("canonfs: unknown action '" + action + "'. Run 't81 help canonfs'.");
  return 1;
}
