// SPDX-License-Identifier: MIT
// RFC-0033: Dual TUI Frontends — shared utility implementations
#include "tooling/tui/common.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace t81::tui {

// ── Command palette ────────────────────────────────────────────────────────

std::vector<CommandEntry> all_commands() {
    return {
        // Code lifecycle
        {"code check",        "Syntax-check a .t81 source file",           "t81 code check"},
        {"code build",        "Compile .t81 → TISC bytecode",              "t81 code build"},
        {"code run",          "Compile and execute a .t81 file",           "t81 code run"},
        {"code lint",         "Lint a .t81 source file",                   "t81 code lint"},
        {"code fmt",          "Format a .t81 source file",                 "t81 code fmt"},
        {"code disasm",       "Disassemble TISC bytecode",                 "t81 code disasm"},
        {"code test",         "Run tests for a .t81 file",                 "t81 code test"},
        // Determinism & tracing
        {"determinism hash",  "Hash-verify a file for determinism",        "t81 determinism hash"},
        {"trace show",        "Show an execution trace",                   "t81 trace show"},
        {"trace diff",        "Diff two execution traces",                 "t81 trace diff"},
        {"trace summary",     "Summarise an execution trace",              "t81 trace summary"},
        {"trace filter",      "Filter trace by opcode or tier",            "t81 trace filter"},
        {"trace replay",      "Replay a canonical trace",                  "t81 trace replay"},
        // CanonFS
        {"canonfs list",      "List all CanonFS artifacts",                "t81 canonfs list"},
        {"canonfs snapshot",  "Snapshot current CanonFS state",            "t81 canonfs snapshot"},
        // Axion / policy
        {"axion status",      "Show active Axion policy status",           "t81 axion status"},
        {"axion log",         "View the Axion audit log",                  "t81 axion log"},
        {"axion audit",       "Full Axion audit report",                   "t81 axion audit"},
        {"axion simulate",    "Simulate policy evaluation",                "t81 axion simulate"},
        {"policy validate",   "Validate an Axion policy file",             "t81 policy validate"},
        {"policy list",       "List all active Axion policies",            "t81 policy list"},
        {"policy compile",    "Compile a policy source file",              "t81 policy compile"},
        // Model weights
        {"weights info",      "Inspect a T81W model artifact",             "t81 weights info"},
        {"weights verify",    "Verify model weight hash (TLOADHASH)",      "t81 weights verify"},
        {"weights quantize",  "Quantize model weights to T3K format",      "t81 weights quantize"},
        // VM / ISA
        {"vm",                "VM introspection subcommands",              "t81 vm"},
        {"tier",              "Cognitive tier operations",                 "t81 tier"},
        // Environment & toolchain
        {"env check",         "Check the T81 runtime environment",         "t81 env check"},
        {"env doctor",        "Diagnose toolchain issues",                 "t81 env doctor"},
        {"version",           "Show T81 version and build info",           "t81 version"},
        {"help",              "Show the full CLI reference",               "t81 help"},
    };
}

// ── Subprocess helper ──────────────────────────────────────────────────────

std::string exec_command(const std::string& cmd) {
    std::string result;
    // NOLINTNEXTLINE(cert-env33-c) — intentional subprocess execution
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "[error: could not launch subprocess]";
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe) != nullptr)
        result += buf;
    pclose(pipe);
    // Trim trailing newline for single-line results
    while (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result.empty() ? "[no output]" : result;
}

// ── Session persistence ────────────────────────────────────────────────────

static std::string escape_json_string(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

static std::string role_str(Message::Role r) {
    switch (r) {
    case Message::Role::User:   return "user";
    case Message::Role::Agent:  return "agent";
    case Message::Role::System: return "system";
    }
    return "system";
}

bool save_session(const std::string& path, const std::vector<Message>& msgs) {
    std::ofstream f(path);
    if (!f) return false;
    for (const auto& m : msgs) {
        f << "{\"role\":\"" << role_str(m.role)
          << "\",\"text\":\"" << escape_json_string(m.text) << "\"}\n";
    }
    return f.good();
}

bool load_session(const std::string& path, std::vector<Message>& msgs) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        // Minimal JSONL parse: just extract role and text values
        auto extract = [&](const std::string& key) -> std::string {
            const std::string needle = "\"" + key + "\":\"";
            auto pos = line.find(needle);
            if (pos == std::string::npos) return {};
            pos += needle.size();
            std::string val;
            for (; pos < line.size(); ++pos) {
                if (line[pos] == '\\' && pos + 1 < line.size()) {
                    char esc = line[++pos];
                    switch (esc) {
                    case '"':  val += '"';  break;
                    case '\\': val += '\\'; break;
                    case 'n':  val += '\n'; break;
                    case 'r':  val += '\r'; break;
                    case 't':  val += '\t'; break;
                    default:   val += esc;  break;
                    }
                } else if (line[pos] == '"') {
                    break;
                } else {
                    val += line[pos];
                }
            }
            return val;
        };
        auto role_s = extract("role");
        auto text   = extract("text");
        Message::Role r = Message::Role::System;
        if (role_s == "user")  r = Message::Role::User;
        if (role_s == "agent") r = Message::Role::Agent;
        msgs.push_back({r, text});
    }
    return true;
}

} // namespace t81::tui
