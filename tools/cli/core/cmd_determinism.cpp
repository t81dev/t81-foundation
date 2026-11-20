#include "tools/cli/core/cmd_determinism.hpp"

#include <algorithm>
#include <chrono>
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
#include <vector>

#include "t81/cli/driver.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"
#include "tools/cli/core/help_text.hpp"
#include "tools/cli/logging.hpp"

namespace fs = std::filesystem;

std::string json_escape(std::string_view text);
std::string render_trace_for_determinism(const t81::vm::State& state);
std::string sha3_512_text(std::string_view text);
std::string sha3_512_file(const fs::path& path);
int run_repro_hash(const char* command_name, const Args& args);
int run_tensor_command(const char* command_name, const Args& args);

namespace {

struct ScopedStreamRedirect {
  std::ostream& from;
  std::streambuf* old_buf;

  ScopedStreamRedirect(std::ostream& from_stream, std::ostream& to_stream)
      : from(from_stream), old_buf(from_stream.rdbuf(to_stream.rdbuf())) {}

  ~ScopedStreamRedirect() { from.rdbuf(old_buf); }
};

template <typename Fn>
int emit_help(Fn&& fn, int rc = 0) {
  ScopedStreamRedirect redirect(std::cerr, std::cout);
  fn();
  return rc;
}

}  // namespace

struct DeterminismRunRecord {
  bool ok = false;
  std::optional<t81::vm::Trap> trap;
  std::string artifact_sha3_512;
  std::string output_sha3_512;
  std::string trace_sha3_512;
};

struct DeterminismBaselineEntry {
  std::string relative_path;
  DeterminismRunRecord record;
};

bool capture_determinism_run_record(const fs::path& input,
                                    const std::optional<fs::path>& policy_path,
                                    DeterminismRunRecord& record, std::string& error_message) {
  try {
    auto program = t81::tisc::load_program(input.string());
    if (policy_path) {
      std::ifstream ifs(*policy_path);
      if (!ifs) {
        error_message = "Could not open policy file: " + policy_path->string();
        return false;
      }
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      program.axion_policy_text = content;
    }
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    std::string printed;
    for (const auto& line : vm->state().printed_output) {
      printed += line;
      printed.push_back('\n');
    }

    record.ok = static_cast<bool>(result);
    if (!result) {
      record.trap = result.error();
    }
    record.artifact_sha3_512 = sha3_512_file(input);
    record.output_sha3_512 = sha3_512_text(printed);
    record.trace_sha3_512 = sha3_512_text(render_trace_for_determinism(vm->state()));
    return true;
  } catch (const std::exception& e) {
    error_message = e.what();
    return false;
  }
}

std::vector<DeterminismBaselineEntry> collect_determinism_baseline_entries(
    const fs::path& source_dir, const std::optional<fs::path>& policy_path,
    std::string& error_message) {
  std::vector<DeterminismBaselineEntry> entries;
  std::error_code ec;
  for (const auto& entry : fs::recursive_directory_iterator(source_dir, ec)) {
    if (ec) {
      error_message = "determinism baseline: failed scanning " + source_dir.string();
      return {};
    }
    if (!entry.is_regular_file() || entry.path().extension() != ".tisc") {
      continue;
    }
    DeterminismBaselineEntry item;
    item.relative_path = fs::relative(entry.path(), source_dir).generic_string();
    if (!capture_determinism_run_record(entry.path(), policy_path, item.record, error_message)) {
      return {};
    }
    entries.push_back(std::move(item));
  }
  std::sort(entries.begin(), entries.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.relative_path < rhs.relative_path; });
  return entries;
}

std::string json_nullable_string(const std::optional<std::string>& value) {
  if (!value) {
    return "null";
  }
  return "\"" + json_escape(*value) + "\"";
}

bool parse_determinism_baseline_file(const fs::path& baseline_file, fs::path& source_dir,
                                     std::vector<DeterminismBaselineEntry>& entries,
                                     std::string& error_message) {
  std::ifstream in(baseline_file, std::ios::binary);
  if (!in) {
    error_message = "Could not open baseline file: " + baseline_file.string();
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::smatch source_match;
  if (!std::regex_search(text, source_match,
                         std::regex(R"json("source_dir"\s*:\s*"([^"]+)")json")) ||
      source_match.size() != 2) {
    error_message = "determinism baseline: missing source_dir in baseline.json";
    return false;
  }
  source_dir = fs::path(source_match[1].str());

  const std::regex entry_re(
      R"json(\{"relative_path":\s*"([^"]+)",\s*"ok":\s*(true|false),\s*"trap":\s*(null|"([^"]*)"),\s*"artifact_sha3_512":\s*"([^"]+)",\s*"output_sha3_512":\s*"([^"]+)",\s*"trace_sha3_512":\s*"([^"]+)"\})json");
  auto begin = std::sregex_iterator(text.begin(), text.end(), entry_re);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    const std::smatch& match = *it;
    DeterminismBaselineEntry entry;
    entry.relative_path = match[1].str();
    entry.record.ok = (match[2].str() == "true");
    if (match[4].matched && !match[4].str().empty()) {
      const std::string trap_name = match[4].str();
      if (trap_name == "DecodeFault")
        entry.record.trap = t81::vm::Trap::DecodeFault;
      else if (trap_name == "TypeFault")
        entry.record.trap = t81::vm::Trap::TypeFault;
      else if (trap_name == "BoundsFault")
        entry.record.trap = t81::vm::Trap::BoundsFault;
      else if (trap_name == "StackFault")
        entry.record.trap = t81::vm::Trap::StackFault;
      else if (trap_name == "DivisionFault")
        entry.record.trap = t81::vm::Trap::DivisionFault;
      else if (trap_name == "SecurityFault")
        entry.record.trap = t81::vm::Trap::SecurityFault;
      else if (trap_name == "TierFault")
        entry.record.trap = t81::vm::Trap::TierFault;
      else if (trap_name == "ShapeFault")
        entry.record.trap = t81::vm::Trap::ShapeFault;
      else if (trap_name == "TrapInstruction")
        entry.record.trap = t81::vm::Trap::TrapInstruction;
      else if (trap_name == "Unimplemented")
        entry.record.trap = t81::vm::Trap::Unimplemented;
      else if (trap_name == "AssertionFailed")
        entry.record.trap = t81::vm::Trap::AssertionFailed;
      else if (trap_name == "EthicsViolation")
        entry.record.trap = t81::vm::Trap::EthicsViolation;
      else if (trap_name == "CapabilityDenied")
        entry.record.trap = t81::vm::Trap::CapabilityDenied;
      else if (trap_name == "None")
        entry.record.trap = t81::vm::Trap::None;
      else {
        error_message = "determinism baseline: unknown trap value '" + trap_name + "'";
        return false;
      }
    }
    entry.record.artifact_sha3_512 = match[5].str();
    entry.record.output_sha3_512 = match[6].str();
    entry.record.trace_sha3_512 = match[7].str();
    entries.push_back(std::move(entry));
  }

  if (entries.empty()) {
    error_message = "determinism baseline: baseline.json does not contain any entries";
    return false;
  }
  return true;
}

int run_determinism_command(const char* command_name, const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_determinism(); });
  }

  const std::string action = args.command_args[0];
  bool as_json = false;
  std::optional<fs::path> policy_path;
  std::optional<fs::path> source_dir_opt;
  std::size_t multi_run_count = 0;
  std::vector<std::string> positional;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--policy") {
      if (++i >= args.command_args.size()) {
        error("determinism: missing value for --policy.");
        return 1;
      }
      policy_path = fs::path(args.command_args[i]);
    } else if (token == "--source-dir") {
      if (++i >= args.command_args.size()) {
        error("determinism: missing value for --source-dir.");
        return 1;
      }
      source_dir_opt = fs::path(args.command_args[i]);
    } else if (token == "--count") {
      if (++i >= args.command_args.size()) {
        error("determinism: missing value for --count.");
        return 1;
      }
      try {
        multi_run_count = static_cast<std::size_t>(std::stoull(args.command_args[i]));
      } catch (...) {
        error("determinism: invalid value for --count.");
        return 1;
      }
    } else if (!token.empty() && token[0] == '-') {
      error("determinism: unknown option '" + token + "'. Run 't81 help determinism'.");
      return 1;
    } else {
      positional.push_back(token);
    }
  }

  if (action == "verify") {
    if (!positional.empty()) {
      const fs::path baseline_dir = fs::path(positional[0]);
      const fs::path baseline_file = baseline_dir / "baseline.json";
      if (fs::exists(baseline_file)) {
        fs::path source_dir;
        std::vector<DeterminismBaselineEntry> baseline_entries;
        std::string parse_error;
        if (!parse_determinism_baseline_file(baseline_file, source_dir, baseline_entries,
                                             parse_error)) {
          error(parse_error);
          return 1;
        }

        std::size_t mismatches = 0;
        std::vector<std::string> mismatch_paths;
        for (const auto& entry : baseline_entries) {
          const fs::path program_path = source_dir / fs::path(entry.relative_path);
          if (!fs::exists(program_path)) {
            ++mismatches;
            mismatch_paths.push_back(entry.relative_path + ": missing");
            continue;
          }
          DeterminismRunRecord current;
          std::string run_error;
          if (!capture_determinism_run_record(program_path, policy_path, current, run_error)) {
            ++mismatches;
            mismatch_paths.push_back(entry.relative_path + ": " + run_error);
            continue;
          }
          if (current.ok != entry.record.ok || current.trap != entry.record.trap ||
              current.artifact_sha3_512 != entry.record.artifact_sha3_512 ||
              current.output_sha3_512 != entry.record.output_sha3_512 ||
              current.trace_sha3_512 != entry.record.trace_sha3_512) {
            ++mismatches;
            mismatch_paths.push_back(entry.relative_path);
          }
        }

        const bool ok = (mismatches == 0);
        if (as_json) {
          std::cout << "{\n"
                    << "  \"schema\": \"t81.determinism-verify.v1\",\n"
                    << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                    << "  \"baseline\": \"" << json_escape(baseline_file.string()) << "\",\n"
                    << "  \"source_dir\": \"" << json_escape(source_dir.string()) << "\",\n"
                    << "  \"checked\": " << baseline_entries.size() << ",\n"
                    << "  \"mismatches\": " << mismatches << ",\n"
                    << "  \"paths\": [\n";
          for (std::size_t i = 0; i < mismatch_paths.size(); ++i) {
            std::cout << "    \"" << json_escape(mismatch_paths[i]) << "\"";
            if (i + 1 < mismatch_paths.size()) {
              std::cout << ",";
            }
            std::cout << "\n";
          }
          std::cout << "  ]\n}\n";
        } else {
          std::cout << "Baseline:   " << baseline_file.string() << "\n";
          std::cout << "Source dir: " << source_dir.string() << "\n";
          std::cout << "Checked:    " << baseline_entries.size() << "\n";
          std::cout << "Mismatches: " << mismatches << "\n";
          for (const auto& mismatch : mismatch_paths) {
            std::cout << "  " << mismatch << "\n";
          }
        }
        return ok ? 0 : 1;
      }
    }
    Args repro_args = args;
    repro_args.command_args = positional;
    return run_repro_hash(command_name, repro_args);
  }
  if (action == "verify-run" || action == "compare-run" || action == "certify" ||
      action == "explain") {
    if (positional.size() != 1) {
      error("determinism " + action + " requires exactly one .tisc input file.");
      return 1;
    }
    const fs::path input = positional[0];
    if (input.extension() != ".tisc") {
      error("determinism " + action + " expects a .tisc input file.");
      return 1;
    }
    struct RunSnapshot {
      bool ok = false;
      std::optional<t81::vm::Trap> trap;
      std::string printed;
      std::string output_hash;
      std::string trace_text;
      std::string trace_hash;
      std::vector<std::string> axion_reasons;
    };
    auto run_once = [&]() -> RunSnapshot {
      RunSnapshot snapshot;
      auto program = t81::tisc::load_program(input.string());
      if (policy_path) {
        std::ifstream ifs(*policy_path);
        if (!ifs) {
          throw std::runtime_error("Could not open policy file: " + policy_path->string());
        }
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        program.axion_policy_text = content;
      }
      auto vm = t81::vm::make_interpreter_vm();
      vm->load_program(program);
      auto result = vm->run_to_halt();
      if (!result) {
        snapshot.trap = result.error();
      } else {
        snapshot.trap.reset();
      }
      for (const auto& line : vm->state().printed_output) {
        snapshot.printed += line;
        snapshot.printed.push_back('\n');
      }
      snapshot.trace_text = render_trace_for_determinism(vm->state());
      snapshot.trace_hash = sha3_512_text(snapshot.trace_text);
      snapshot.output_hash = sha3_512_text(snapshot.printed);
      for (const auto& event : vm->state().axion_log) {
        snapshot.axion_reasons.push_back(event.verdict.reason);
      }
      snapshot.ok = static_cast<bool>(result);
      return snapshot;
    };
    const RunSnapshot run1 = run_once();
    const RunSnapshot run2 = run_once();
    const bool identical = (run1.ok == run2.ok) && (run1.trap == run2.trap) &&
                           (run1.output_hash == run2.output_hash) &&
                           (run1.trace_hash == run2.trace_hash) &&
                           (run1.axion_reasons == run2.axion_reasons);
    std::string reason = "identical";
    std::optional<std::size_t> mismatch_index;
    std::optional<std::string> lhs_detail;
    std::optional<std::string> rhs_detail;
    if (run1.ok != run2.ok) {
      reason = "status_mismatch";
      lhs_detail = run1.ok ? "ok" : "trap";
      rhs_detail = run2.ok ? "ok" : "trap";
    } else if (run1.trap != run2.trap) {
      reason = "trap_mismatch";
      lhs_detail = run1.trap ? t81::vm::to_string(*run1.trap) : "null";
      rhs_detail = run2.trap ? t81::vm::to_string(*run2.trap) : "null";
    } else if (run1.output_hash != run2.output_hash) {
      reason = "output_mismatch";
      lhs_detail = run1.printed;
      rhs_detail = run2.printed;
    } else {
      auto split_lines_local = [](std::string_view text) {
        std::vector<std::string> lines;
        std::string current;
        for (char ch : text) {
          if (ch == '\n') {
            lines.push_back(current);
            current.clear();
          } else {
            current.push_back(ch);
          }
        }
        if (!current.empty()) {
          lines.push_back(current);
        }
        return lines;
      };
      const auto trace_lines_1 = split_lines_local(run1.trace_text);
      const auto trace_lines_2 = split_lines_local(run2.trace_text);
      const std::size_t shared = std::min(trace_lines_1.size(), trace_lines_2.size());
      for (std::size_t i = 0; i < shared; ++i) {
        if (trace_lines_1[i] != trace_lines_2[i]) {
          reason = "trace_entry_mismatch";
          mismatch_index = i;
          lhs_detail = trace_lines_1[i];
          rhs_detail = trace_lines_2[i];
          break;
        }
      }
      if (reason == "identical" && trace_lines_1.size() != trace_lines_2.size()) {
        reason = "trace_size_mismatch";
        mismatch_index = shared;
        lhs_detail = std::to_string(trace_lines_1.size());
        rhs_detail = std::to_string(trace_lines_2.size());
      }
      if (reason == "identical") {
        const std::size_t axion_shared =
            std::min(run1.axion_reasons.size(), run2.axion_reasons.size());
        for (std::size_t i = 0; i < axion_shared; ++i) {
          if (run1.axion_reasons[i] != run2.axion_reasons[i]) {
            reason = "axion_reason_mismatch";
            mismatch_index = i;
            lhs_detail = run1.axion_reasons[i];
            rhs_detail = run2.axion_reasons[i];
            break;
          }
        }
        if (reason == "identical" && run1.axion_reasons.size() != run2.axion_reasons.size()) {
          reason = "axion_event_count_mismatch";
          mismatch_index = axion_shared;
          lhs_detail = std::to_string(run1.axion_reasons.size());
          rhs_detail = std::to_string(run2.axion_reasons.size());
        }
      }
    }

    if (action == "certify") {
      const std::string program_hash = sha3_512_file(input);
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.determinism-certificate.v1\",\n"
                  << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                  << "  \"program_sha3_512\": \"" << program_hash << "\",\n"
                  << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                  << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                  << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                  << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\",\n"
                  << "  \"deterministic\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"trap\": ";
        if (run1.trap) {
          std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\"\n";
        } else {
          std::cout << "null\n";
        }
        std::cout << "}\n";
      } else {
        std::cout << "program_sha3_512=" << program_hash << "\n";
        std::cout << "output_sha3_512=" << run1.output_hash << "\n";
        std::cout << "trace_sha3_512=" << run1.trace_hash << "\n";
        std::cout << "deterministic=" << (identical ? "true" : "false") << "\n";
        if (run1.trap) {
          std::cout << "trap=" << t81::vm::to_string(*run1.trap) << "\n";
        }
      }
      return identical ? 0 : 1;
    }
    if (action == "verify-run") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.determinism-verify-run.v1\",\n"
                  << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                  << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                  << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                  << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                  << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\"";
        if (run1.trap || run2.trap) {
          std::cout << ",\n  \"run1_trap\": ";
          if (run1.trap)
            std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\"";
          else
            std::cout << "null";
          std::cout << ",\n  \"run2_trap\": ";
          if (run2.trap)
            std::cout << "\"" << json_escape(t81::vm::to_string(*run2.trap)) << "\"";
          else
            std::cout << "null";
        }
        std::cout << "\n}\n";
      } else if (identical) {
        std::cout << "deterministic-run-verified\n";
      } else {
        std::cout << "deterministic-run-mismatch\n";
      }
      return identical ? 0 : 1;
    }
    if (action == "compare-run") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.determinism-compare-run.v1\",\n"
                  << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                  << "  \"reason\": \"" << json_escape(reason) << "\",\n"
                  << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                  << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                  << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                  << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\",\n"
                  << "  \"mismatch_index\": ";
        if (mismatch_index)
          std::cout << *mismatch_index << ",\n";
        else
          std::cout << "null,\n";
        std::cout << "  \"lhs_detail\": ";
        if (lhs_detail)
          std::cout << "\"" << json_escape(*lhs_detail) << "\",\n";
        else
          std::cout << "null,\n";
        std::cout << "  \"rhs_detail\": ";
        if (rhs_detail)
          std::cout << "\"" << json_escape(*rhs_detail) << "\",\n";
        else
          std::cout << "null,\n";
        std::cout << "  \"run1_trap\": ";
        if (run1.trap)
          std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\",\n";
        else
          std::cout << "null,\n";
        std::cout << "  \"run2_trap\": ";
        if (run2.trap)
          std::cout << "\"" << json_escape(t81::vm::to_string(*run2.trap)) << "\"\n";
        else
          std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Program:   " << input.string() << "\n";
        std::cout << "Result:    " << (identical ? "identical" : reason) << "\n";
        if (mismatch_index) std::cout << "Index:     " << *mismatch_index << "\n";
        if (lhs_detail) std::cout << "Run1:      " << *lhs_detail << "\n";
        if (rhs_detail) std::cout << "Run2:      " << *rhs_detail << "\n";
      }
      return identical ? 0 : 1;
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-explain.v1\",\n"
                << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"reason\": \"" << json_escape(reason) << "\",\n"
                << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\",\n"
                << "  \"mismatch_index\": ";
      if (mismatch_index)
        std::cout << *mismatch_index << ",\n";
      else
        std::cout << "null,\n";
      std::cout << "  \"lhs_detail\": ";
      if (lhs_detail)
        std::cout << "\"" << json_escape(*lhs_detail) << "\",\n";
      else
        std::cout << "null,\n";
      std::cout << "  \"rhs_detail\": ";
      if (rhs_detail)
        std::cout << "\"" << json_escape(*rhs_detail) << "\",\n";
      else
        std::cout << "null,\n";
      std::cout << "  \"run1_trap\": ";
      if (run1.trap)
        std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\",\n";
      else
        std::cout << "null,\n";
      std::cout << "  \"run2_trap\": ";
      if (run2.trap)
        std::cout << "\"" << json_escape(t81::vm::to_string(*run2.trap)) << "\"\n";
      else
        std::cout << "null\n";
      std::cout << "}\n";
    } else if (identical) {
      std::cout << "determinism-explain: runs are identical\n";
    } else {
      std::cout << "determinism-explain: " << reason << "\n";
      if (mismatch_index) {
        std::cout << "Mismatch index: " << *mismatch_index << "\n";
      }
      if (lhs_detail) {
        std::cout << "Run1: " << *lhs_detail << "\n";
      }
      if (rhs_detail) {
        std::cout << "Run2: " << *rhs_detail << "\n";
      }
    }
    return identical ? 0 : 1;
  }
  if (action == "multi-run") {
    if (positional.size() != 1) {
      error("determinism multi-run requires exactly one .tisc input file.");
      return 1;
    }
    if (multi_run_count == 0) {
      error("determinism multi-run requires --count <n> with n >= 1.");
      return 1;
    }
    const fs::path input = positional[0];
    if (input.extension() != ".tisc") {
      error("determinism multi-run expects a .tisc input file.");
      return 1;
    }

    std::vector<std::string> signatures;
    std::optional<std::string> baseline;
    bool ok = true;
    std::size_t mismatch_run = 0;
    for (std::size_t run = 0; run < multi_run_count; ++run) {
      auto program = t81::tisc::load_program(input.string());
      if (policy_path) {
        std::ifstream ifs(*policy_path);
        if (!ifs) {
          error("Could not open policy file: " + policy_path->string());
          return 1;
        }
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        program.axion_policy_text = content;
      }
      auto vm = t81::vm::make_interpreter_vm();
      vm->load_program(program);
      auto result = vm->run_to_halt();
      std::string printed;
      for (const auto& line : vm->state().printed_output) {
        printed += line;
        printed.push_back('\n');
      }
      const auto signature =
          sha3_512_text(render_trace_for_determinism(vm->state()) + "\n--\n" + printed);
      signatures.push_back(signature);
      if (!result) {
        ok = false;
        mismatch_run = run + 1;
        break;
      }
      if (!baseline) {
        baseline = signature;
      } else if (*baseline != signature) {
        ok = false;
        mismatch_run = run + 1;
        break;
      }
    }

    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-multi-run.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"count\": " << multi_run_count << ",\n"
                << "  \"completed\": " << signatures.size() << ",\n"
                << "  \"mismatch_run\": " << (mismatch_run == 0 ? 0 : mismatch_run) << ",\n"
                << "  \"signatures\": [\n";
      for (std::size_t i = 0; i < signatures.size(); ++i) {
        std::cout << "    \"" << json_escape(signatures[i]) << "\"";
        if (i + 1 < signatures.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "Program:    " << input.string() << "\n";
      std::cout << "Runs:       " << multi_run_count << "\n";
      std::cout << "Completed:  " << signatures.size() << "\n";
      std::cout << "Determinism:" << (ok ? " stable" : " diverged") << "\n";
      if (mismatch_run != 0) {
        std::cout << "Mismatch:   run " << mismatch_run << "\n";
      }
    }
    return ok ? 0 : 1;
  }
  if (action == "hash") {
    if (positional.size() != 1) {
      error("determinism hash requires exactly one input file.");
      return 1;
    }
    return t81::cli::determinism_hash_file(positional[0], as_json);
  }
  if (action == "trace-hash") {
    if (positional.size() != 1) {
      error("determinism trace-hash requires exactly one trace file.");
      return 1;
    }
    return t81::cli::determinism_hash_trace(positional[0], as_json);
  }
  if (action == "diff") {
    if (positional.size() != 2) {
      error("determinism diff requires exactly two input files.");
      return 1;
    }
    return t81::cli::determinism_diff_files(positional[0], positional[1], as_json);
  }
  if (action == "diff-trace") {
    if (positional.size() != 2) {
      error("determinism diff-trace requires exactly two trace files.");
      return 1;
    }
    return t81::cli::determinism_diff_trace_files(positional[0], positional[1], as_json);
  }

  if (action == "baseline") {
    if (positional.empty()) {
      error("determinism baseline requires an output directory. Run 't81 help determinism "
            "baseline'.");
      return 1;
    }
    const fs::path out_dir = fs::path(positional[0]);
    const fs::path source_dir = source_dir_opt.value_or(out_dir);
    std::error_code ec;
    fs::create_directories(out_dir, ec);
    std::string collect_error;
    const auto entries =
        collect_determinism_baseline_entries(source_dir, policy_path, collect_error);
    if (!collect_error.empty()) {
      error(collect_error);
      return 1;
    }
    const fs::path baseline_file = out_dir / "baseline.json";
    std::ofstream ofs(baseline_file, std::ios::trunc);
    if (!ofs) {
      error("determinism baseline: could not open output file: " + baseline_file.string());
      return 1;
    }
    ofs << "{\n"
        << "  \"schema\": \"t81.determinism-baseline.v1\",\n"
        << "  \"source_dir\": \"" << json_escape(source_dir.string()) << "\",\n"
        << "  \"entries\": [\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      ofs << "    {\"relative_path\": \"" << json_escape(entries[i].relative_path)
          << "\", \"ok\": " << (entries[i].record.ok ? "true" : "false") << ", \"trap\": "
          << json_nullable_string(
                 entries[i].record.trap
                     ? std::optional<std::string>{t81::vm::to_string(*entries[i].record.trap)}
                     : std::nullopt)
          << ", \"artifact_sha3_512\": \"" << entries[i].record.artifact_sha3_512
          << "\", \"output_sha3_512\": \"" << entries[i].record.output_sha3_512
          << "\", \"trace_sha3_512\": \"" << entries[i].record.trace_sha3_512 << "\"}";
      if (i + 1 < entries.size()) ofs << ",";
      ofs << "\n";
    }
    ofs << "  ]\n}\n";
    if (!ofs.good()) {
      error("determinism baseline: failed writing " + baseline_file.string());
      return 1;
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-baseline.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"baseline\": \"" << json_escape(baseline_file.string()) << "\",\n"
                << "  \"source_dir\": \"" << json_escape(source_dir.string()) << "\",\n"
                << "  \"count\": " << entries.size() << "\n"
                << "}\n";
    } else {
      std::cout << "Baseline written: " << baseline_file.string() << " (" << entries.size()
                << " entries)\n";
    }
    return 0;
  }

  if (action == "bisect") {
    if (positional.size() != 1) {
      error("determinism bisect requires a baseline directory. Run 't81 help determinism bisect'.");
      return 1;
    }
    const fs::path baseline_dir = fs::path(positional[0]);
    const fs::path baseline_file = baseline_dir / "baseline.json";
    if (!fs::exists(baseline_file)) {
      error("determinism bisect: baseline.json not found under " + baseline_dir.string());
      return 1;
    }
    fs::path source_dir;
    std::vector<DeterminismBaselineEntry> baseline_entries;
    std::string parse_error;
    if (!parse_determinism_baseline_file(baseline_file, source_dir, baseline_entries,
                                         parse_error)) {
      error(parse_error);
      return 1;
    }
    bool ok = true;
    std::optional<std::string> first_path;
    std::optional<std::string> reason;
    std::optional<DeterminismRunRecord> current;
    std::optional<DeterminismRunRecord> expected;
    for (const auto& entry : baseline_entries) {
      const fs::path current_file = source_dir / entry.relative_path;
      if (!fs::exists(current_file)) {
        ok = false;
        first_path = entry.relative_path;
        reason = "missing_artifact";
        expected = entry.record;
        break;
      }
      DeterminismRunRecord current_record;
      std::string capture_error;
      if (!capture_determinism_run_record(current_file, policy_path, current_record,
                                          capture_error)) {
        ok = false;
        first_path = entry.relative_path;
        reason = "execution_error";
        expected = entry.record;
        break;
      }
      if (current_record.ok != entry.record.ok) {
        ok = false;
        first_path = entry.relative_path;
        reason = "status_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.trap != entry.record.trap) {
        ok = false;
        first_path = entry.relative_path;
        reason = "trap_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.artifact_sha3_512 != entry.record.artifact_sha3_512) {
        ok = false;
        first_path = entry.relative_path;
        reason = "artifact_hash_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.output_sha3_512 != entry.record.output_sha3_512) {
        ok = false;
        first_path = entry.relative_path;
        reason = "output_hash_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.trace_sha3_512 != entry.record.trace_sha3_512) {
        ok = false;
        first_path = entry.relative_path;
        reason = "trace_hash_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-bisect.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"baseline\": \"" << json_escape(baseline_file.string()) << "\",\n"
                << "  \"first_mismatch\": ";
      if (first_path)
        std::cout << "\"" << json_escape(*first_path) << "\",\n";
      else
        std::cout << "null,\n";
      std::cout << "  \"reason\": ";
      if (reason)
        std::cout << "\"" << json_escape(*reason) << "\",\n";
      else
        std::cout << "null,\n";
      std::cout << "  \"expected_trace_sha3_512\": "
                << json_nullable_string(expected
                                            ? std::optional<std::string>{expected->trace_sha3_512}
                                            : std::nullopt)
                << ",\n";
      std::cout << "  \"current_trace_sha3_512\": "
                << json_nullable_string(
                       current ? std::optional<std::string>{current->trace_sha3_512} : std::nullopt)
                << "\n}\n";
    } else if (ok) {
      std::cout << "No baseline mismatch found.\n";
    } else {
      std::cout << "First mismatch: " << *first_path << "\n";
      std::cout << "Reason:         " << *reason << "\n";
    }
    return ok ? 0 : 1;
  }

  error("determinism: unknown action '" + action + "'. Run 't81 help determinism'.");
  return 1;
}
