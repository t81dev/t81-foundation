#include "tools/cli/core/cmd_policy.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "t81/axion/policy_validator.hpp"
#include "t81/cli/driver.hpp"
#include "t81/vm/vm.hpp"
#include "tools/cli/core/help_text.hpp"
#include "tools/cli/logging.hpp"

namespace fs = std::filesystem;

std::string json_escape(std::string_view text);
fs::path discover_repo_root();

int run_policy_compile(const Args& args) {
  if (args.command_args.size() < 2) {
    error("policy compile requires an input .apl file. Run 't81 help policy compile'.");
    return 1;
  }
  fs::path input = args.command_args[1];
  fs::path output = args.output.value_or(input.stem().string() + ".axionb");
  std::ifstream ifs(input);
  if (!ifs) {
    error("Could not open input file: " + input.string());
    return 1;
  }
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  auto policy_res = t81::axion::parse_policy(content);
  if (!policy_res) {
    error("Policy parse error: " + policy_res.error());
    return 1;
  }
  auto& policy = policy_res.value();
  policy.compile_to_bytecode();
  std::ofstream ofs(output, std::ios::binary);
  if (!ofs) {
    error("Could not open output file: " + output.string());
    return 1;
  }
  policy.serialize(ofs);
  info("Compiled policy to " + output.string());
  return 0;
}

int run_policy_run(const Args& args) {
  if (args.command_args.size() < 2) {
    error("policy run requires an input .apl or .axionb file. Run 't81 help policy run'.");
    return 1;
  }
  fs::path input;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("policy run: unknown option '" + token + "'. Run 't81 help policy run'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("policy run: unexpected argument '" + token + "'. Run 't81 help policy run'.");
      return 1;
    }
  }
  if (input.empty()) {
    error("policy run requires an input .apl or .axionb file. Run 't81 help policy run'.");
    return 1;
  }
  auto emit_json_error = [&](std::string_view message) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.policy-run.v1\",\n";
    std::cout << "  \"valid\": false,\n";
    std::cout << "  \"policy\": \"" << json_escape(input.string()) << "\",\n";
    std::cout << "  \"error\": \"" << json_escape(message) << "\"\n";
    std::cout << "}\n";
  };
  t81::axion::Policy policy;
  if (input.extension() == ".axionb") {
    std::ifstream ifs(input, std::ios::binary);
    if (!ifs) {
      if (as_json) {
        emit_json_error("Could not open policy file: " + input.string());
        return 1;
      }
      error("Could not open policy file: " + input.string());
      return 1;
    }
    auto res = t81::axion::Policy::deserialize(ifs);
    if (!res) {
      if (as_json) {
        emit_json_error("Policy deserialization error: " + res.error());
        return 1;
      }
      error("Policy deserialization error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  } else {
    std::ifstream ifs(input);
    if (!ifs) {
      if (as_json) {
        emit_json_error("Could not open policy file: " + input.string());
        return 1;
      }
      error("Could not open policy file: " + input.string());
      return 1;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto res = t81::axion::parse_policy(content);
    if (!res) {
      if (as_json) {
        emit_json_error("Policy parse error: " + res.error());
        return 1;
      }
      error("Policy parse error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  }
  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.policy-run.v1\",\n";
    std::cout << "  \"valid\": true,\n";
    std::cout << "  \"tier\": " << policy.tier << ",\n";
    if (policy.max_instructions) {
      std::cout << "  \"max_instructions\": " << *policy.max_instructions << ",\n";
    } else {
      std::cout << "  \"max_instructions\": null,\n";
    }
    if (policy.max_stack) {
      std::cout << "  \"max_stack\": " << *policy.max_stack << ",\n";
    } else {
      std::cout << "  \"max_stack\": null,\n";
    }
    if (policy.max_recursion) {
      std::cout << "  \"max_recursion\": " << *policy.max_recursion << "\n";
    } else {
      std::cout << "  \"max_recursion\": null\n";
    }
    std::cout << "}\n";
  } else {
    info("Policy validated successfully.");
    if (g_flags.verbose) {
      std::cout << "Tier: " << policy.tier << "\n";
      if (policy.max_instructions)
        std::cout << "Max Instructions: " << *policy.max_instructions << "\n";
      if (policy.max_stack) std::cout << "Max Stack: " << *policy.max_stack << "\n";
      if (policy.max_recursion) std::cout << "Max Recursion: " << *policy.max_recursion << "\n";
    }
  }
  return 0;
}

int run_policy_validate(const Args& args) {
  if (args.command_args.size() < 2) {
    error(
        "policy validate requires an input .apl or .axionb file. Run 't81 help policy validate'.");
    return 1;
  }
  fs::path input;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("policy validate: unknown option '" + token + "'. Run 't81 help policy validate'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("policy validate: unexpected argument '" + token +
            "'. Run 't81 help policy validate'.");
      return 1;
    }
  }
  if (input.empty()) {
    error(
        "policy validate requires an input .apl or .axionb file. Run 't81 help policy validate'.");
    return 1;
  }
  auto emit_json_error = [&](std::string_view message) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.policy-validate.v1\",\n";
    std::cout << "  \"valid\": false,\n";
    std::cout << "  \"policy\": \"" << json_escape(input.string()) << "\",\n";
    std::cout << "  \"error\": \"" << json_escape(message) << "\"\n";
    std::cout << "}\n";
  };
  t81::axion::Policy policy;
  if (input.extension() == ".axionb") {
    std::ifstream ifs(input, std::ios::binary);
    if (!ifs) {
      if (as_json) {
        emit_json_error("Could not open policy file: " + input.string());
        return 1;
      }
      error("Could not open policy file: " + input.string());
      return 1;
    }
    auto res = t81::axion::Policy::deserialize(ifs);
    if (!res) {
      if (as_json) {
        emit_json_error("Policy deserialization error: " + res.error());
        return 1;
      }
      error("Policy deserialization error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  } else {
    std::ifstream ifs(input);
    if (!ifs) {
      if (as_json) {
        emit_json_error("Could not open policy file: " + input.string());
        return 1;
      }
      error("Could not open policy file: " + input.string());
      return 1;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto res = t81::axion::parse_policy(content);
    if (!res) {
      if (as_json) {
        emit_json_error("Policy parse error: " + res.error());
        return 1;
      }
      error("Policy parse error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  }
  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.policy-validate.v1\",\n";
    std::cout << "  \"valid\": true,\n";
    std::cout << "  \"policy\": \"" << json_escape(input.string()) << "\",\n";
    std::cout << "  \"tier\": " << policy.tier << ",\n";
    if (policy.max_instructions) {
      std::cout << "  \"max_instructions\": " << *policy.max_instructions << ",\n";
    } else {
      std::cout << "  \"max_instructions\": null,\n";
    }
    if (policy.max_stack) {
      std::cout << "  \"max_stack\": " << *policy.max_stack << ",\n";
    } else {
      std::cout << "  \"max_stack\": null,\n";
    }
    if (policy.max_recursion) {
      std::cout << "  \"max_recursion\": " << *policy.max_recursion << "\n";
    } else {
      std::cout << "  \"max_recursion\": null\n";
    }
    std::cout << "}\n";
  } else {
    std::cout << "Policy valid: " << input.string() << "\n";
    if (g_flags.verbose) {
      std::cout << "Tier: " << policy.tier << "\n";
      if (policy.max_instructions)
        std::cout << "Max Instructions: " << *policy.max_instructions << "\n";
      if (policy.max_stack) std::cout << "Max Stack: " << *policy.max_stack << "\n";
      if (policy.max_recursion) std::cout << "Max Recursion: " << *policy.max_recursion << "\n";
    }
  }
  return 0;
}

int run_policy(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_policy(); });
  }
  if (args.command_args.size() >= 2 &&
      (args.command_args[1] == "-h" || args.command_args[1] == "--help")) {
    const std::string sub = args.command_args[0];
    if (sub == "compile") {
      print_help_policy_compile();
      return 0;
    }
    if (sub == "validate") {
      print_help_policy_validate();
      return 0;
    }
    if (sub == "run") {
      print_help_policy_run();
      return 0;
    }
    if (sub == "test") {
      print_help_policy_test();
      return 0;
    }
    if (sub == "list") {
      print_help_policy_list();
      return 0;
    }
    error("policy: unknown subcommand '" + sub + "'. Run 't81 help policy'.");
    return 1;
  }
  if (args.command_args.empty()) {
    error("policy requires a subcommand (compile|validate|run|test|list). Run 't81 help policy'.");
    return 1;
  }
  const std::string sub = args.command_args[0];
  if (sub == "compile") return run_policy_compile(args);
  if (sub == "validate") return run_policy_validate(args);
  if (sub == "run") return run_policy_run(args);
  if (sub == "test") {
    if (args.command_args.size() < 2) {
      error("policy test requires an input .apl or .axionb file. Run 't81 help policy test'.");
      return 1;
    }
    fs::path input;
    std::string model_hash;
    bool as_json = false;
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      const std::string& token = args.command_args[i];
      if (token == "--json") {
        as_json = true;
      } else if (token == "--model-hash") {
        if (++i >= args.command_args.size()) {
          error("policy test: missing value for --model-hash. Run 't81 help policy test'.");
          return 1;
        }
        model_hash = args.command_args[i];
      } else if (!token.empty() && token[0] == '-') {
        error("policy test: unknown option '" + token + "'. Run 't81 help policy test'.");
        return 1;
      } else if (input.empty()) {
        input = fs::path(token);
      } else {
        error("policy test: unexpected argument '" + token + "'. Run 't81 help policy test'.");
        return 1;
      }
    }
    if (input.empty() || model_hash.empty()) {
      error("policy test requires <policy> and --model-hash <hash>. Run 't81 help policy test'.");
      return 1;
    }
    std::ifstream ifs(input, input.extension() == ".axionb" ? std::ios::binary : std::ios::in);
    if (!ifs) {
      error("Could not open policy file: " + input.string());
      return 1;
    }
    std::string policy_text;
    if (input.extension() == ".axionb") {
      auto res = t81::axion::Policy::deserialize(ifs);
      if (!res) {
        error("Policy deserialization error: " + res.error());
        return 1;
      }
      const auto& policy = res.value();
      if (std::find(policy.allowed_tensor_hashes.begin(), policy.allowed_tensor_hashes.end(),
                    model_hash) == policy.allowed_tensor_hashes.end()) {
        if (as_json) {
          std::cout
              << "{\n  \"schema\": \"t81.policy-test.v1\",\n  \"ok\": false,\n  \"model_hash\": \""
              << json_escape(model_hash) << "\",\n  \"rule\": \"hash_not_allowed\"\n}\n";
        } else {
          error("Policy test failed: model hash is not allowed.");
        }
        return 1;
      }
      if (as_json) {
        std::cout
            << "{\n  \"schema\": \"t81.policy-test.v1\",\n  \"ok\": true,\n  \"model_hash\": \""
            << json_escape(model_hash) << "\",\n  \"rule\": \"hash_valid\"\n}\n";
      } else {
        info("Policy test passed.");
      }
      return 0;
    }
    policy_text.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto violation = t81::axion::PolicyValidator::validate_tensor_hash(model_hash, policy_text);
    const bool ok = violation.has_value() && !violation->is_violation;
    if (as_json) {
      std::cout << "{\n  \"schema\": \"t81.policy-test.v1\",\n  \"ok\": " << (ok ? "true" : "false")
                << ",\n  \"model_hash\": \"" << json_escape(model_hash) << "\",\n  \"rule\": \""
                << json_escape(violation ? violation->rule : "unknown") << "\",\n  \"reason\": \""
                << json_escape(violation ? violation->reason : "validation unavailable")
                << "\"\n}\n";
    } else if (violation) {
      std::cout << t81::axion::PolicyValidator::generate_policy_report(*violation) << "\n";
    }
    return ok ? 0 : 1;
  }
  if (sub == "list") {
    bool as_json = false;
    fs::path search_dir = discover_repo_root();
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      if (args.command_args[i] == "--json") {
        as_json = true;
      } else if (args.command_args[i] == "--dir" && i + 1 < args.command_args.size()) {
        search_dir = fs::path(args.command_args[++i]);
      }
    }
    std::vector<fs::path> found;
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(search_dir, ec)) {
      if (ec) break;
      if (entry.is_regular_file() && entry.path().extension() == ".apl") {
        found.push_back(entry.path());
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.policy-list.v1\",\n"
                << "  \"count\": " << found.size() << ",\n"
                << "  \"policies\": [\n";
      for (std::size_t i = 0; i < found.size(); ++i) {
        std::cout << "    \"" << json_escape(found[i].string()) << "\"";
        if (i + 1 < found.size()) std::cout << ",";
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      if (found.empty()) {
        std::cout << "No .apl policy files found in: " << search_dir.string() << "\n";
      } else {
        for (const auto& p : found) std::cout << p.string() << "\n";
      }
    }
    return 0;
  }

  error("policy: unknown subcommand '" + sub + "'. Run 't81 help policy'.");
  return 1;
}
