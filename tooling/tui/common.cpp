// SPDX-License-Identifier: MIT
// T81 TUI — shared utility implementations
#include "tooling/tui/common.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#if !defined(_WIN32)
#  include <sys/wait.h>
#  include <unistd.h>
#endif

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

// ── Small helpers ──────────────────────────────────────────────────────────

std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string line;
    while (std::getline(ss, line))
        out.push_back(line);
    if (out.empty())
        out.push_back(s);
    return out;
}

std::vector<const CommandEntry*> filter_palette(
    const std::vector<CommandEntry>& cmds, const std::string& query)
{
    std::vector<const CommandEntry*> out;
    std::string q = query;
    for (char& c : q)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (const auto& cmd : cmds) {
        std::string name = cmd.name;
        for (char& c : name)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (q.empty() || name.find(q) != std::string::npos)
            out.push_back(&cmd);
    }
    return out;
}

int palette_window_start(int selected, int count, int visible) {
    if (count <= 0 || visible <= 0)
        return 0;
    selected = std::max(0, std::min(selected, count - 1));
    const int max_start = std::max(0, count - visible);
    return std::max(0, std::min(selected - visible + 1, max_start));
}

std::string scroll_indicator_text(
    int scroll_offset, int total_lines, int visible, const std::string& suffix)
{
    const int total = std::max(0, total_lines);
    const int start = (total == 0)
        ? 0
        : std::max(0, std::min(scroll_offset, std::max(0, total - visible)));
    const int end = std::min(total, start + std::max(0, visible));
    const int first = (total == 0) ? 0 : start + 1;
    return " L" + std::to_string(first) + "-" + std::to_string(end) + "/" +
           std::to_string(total) + suffix;
}

std::vector<std::string> split_command_words(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    bool in_single = false;
    bool in_double = false;
    bool escape = false;

    for (char c : text) {
        if (escape) {
            cur += c;
            escape = false;
            continue;
        }
        if (c == '\\' && !in_single) {
            escape = true;
            continue;
        }
        if (c == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        if (c == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (!in_single && !in_double &&
            std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
            continue;
        }
        cur += c;
    }
    if (escape)
        cur += '\\';
    if (!cur.empty())
        out.push_back(cur);
    return out;
}

std::string finalize_process_output(std::string result, int status) {
    while (!result.empty() && result.back() == '\n')
        result.pop_back();
    if (status != 0) {
        if (!result.empty())
            result += '\n';
        result += "[exit status " + std::to_string(status) + "]";
    }
    return result.empty() ? "[no output]" : result;
}

std::string exec_command(const std::string& cmd) {
    std::string result;
    // NOLINTNEXTLINE(cert-env33-c) — intentional subprocess execution
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "[error: could not launch subprocess]";
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe) != nullptr)
        result += buf;
    const int rc = pclose(pipe);
#if defined(_WIN32)
    const int status = rc;
#else
    const int status = (WIFEXITED(rc) ? WEXITSTATUS(rc) : rc);
#endif
    return finalize_process_output(std::move(result), status);
}

std::string exec_argv(const std::vector<std::string>& argv) {
    if (argv.empty()) return "[error: empty command]";

#if defined(_WIN32)
    // Windows fallback: shell-quote each argument and delegate to exec_command.
    // This is weaker than the Unix path but better than bare concatenation.
    std::string cmd;
    for (const auto& arg : argv) {
        if (!cmd.empty()) cmd += ' ';
        // Wrap in double quotes; escape any embedded double quotes.
        cmd += '"';
        for (char c : arg) {
            if (c == '"') cmd += "\\\"";
            else          cmd += c;
        }
        cmd += '"';
    }
    return exec_command(cmd + " 2>&1");
#else
    // Unix path: fork + execvp.  No shell is involved; user-supplied tokens
    // are passed as discrete arguments and cannot inject shell metacharacters.
    int pipefd[2];
    if (pipe(pipefd) < 0) return "[error: pipe failed]";

    const pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return "[error: fork failed]";
    }

    if (pid == 0) {
        // Child: redirect stdout and stderr into the pipe write-end.
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        // Build a null-terminated argv for execvp.
        std::vector<char*> cargv;
        cargv.reserve(argv.size() + 1);
        for (const auto& a : argv)
            cargv.push_back(const_cast<char*>(a.c_str()));
        cargv.push_back(nullptr);

        execvp(cargv[0], cargv.data());
        // exec failed — write a diagnostic and exit.
        const char* msg = "[error: execvp failed]\n";
        (void)write(STDERR_FILENO, msg, std::strlen(msg));
        _exit(127);
    }

    // Parent: drain the pipe then wait for the child.
    close(pipefd[1]);
    std::string result;
    char buf[512];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0)
        result.append(buf, static_cast<size_t>(n));
    close(pipefd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    status = WIFEXITED(status) ? WEXITSTATUS(status) : status;
    return finalize_process_output(std::move(result), status);
#endif
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

namespace {

bool parse_json_string(const std::string& s, size_t& pos, std::string& out)
{
    if (pos >= s.size() || s[pos] != '"')
        return false;
    ++pos;
    out.clear();
    while (pos < s.size()) {
        const char c = s[pos++];
        if (c == '"')
            return true;
        if (c != '\\') {
            out += c;
            continue;
        }
        if (pos >= s.size())
            return false;
        const char esc = s[pos++];
        switch (esc) {
        case '"':  out += '"';  break;
        case '\\': out += '\\'; break;
        case '/':  out += '/';  break;
        case 'n':  out += '\n'; break;
        case 'r':  out += '\r'; break;
        case 't':  out += '\t'; break;
        case 'b':  out += '\b'; break;
        case 'f':  out += '\f'; break;
        default:   return false;
        }
    }
    return false;
}

void skip_ws(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])))
        ++pos;
}

bool parse_session_line(const std::string& line, Message& msg) {
    size_t pos = 0;
    skip_ws(line, pos);
    if (pos >= line.size() || line[pos] != '{')
        return false;
    ++pos;

    std::string role_s;
    std::string text_s;
    bool have_role = false;
    bool have_text = false;

    for (;;) {
        skip_ws(line, pos);
        if (pos < line.size() && line[pos] == '}') {
            ++pos;
            break;
        }

        std::string key;
        if (!parse_json_string(line, pos, key))
            return false;
        skip_ws(line, pos);
        if (pos >= line.size() || line[pos] != ':')
            return false;
        ++pos;
        skip_ws(line, pos);

        std::string value;
        if (!parse_json_string(line, pos, value))
            return false;
        if (key == "role") {
            role_s = value;
            have_role = true;
        } else if (key == "text") {
            text_s = value;
            have_text = true;
        }

        skip_ws(line, pos);
        if (pos < line.size() && line[pos] == ',') {
            ++pos;
            continue;
        }
        if (pos < line.size() && line[pos] == '}') {
            ++pos;
            break;
        }
        return false;
    }

    skip_ws(line, pos);
    if (pos != line.size() || !have_text)
        return false;

    msg.role = Message::Role::System;
    if (have_role && role_s == "user")
        msg.role = Message::Role::User;
    else if (have_role && role_s == "agent")
        msg.role = Message::Role::Agent;
    msg.text = std::move(text_s);
    return true;
}

} // namespace

bool load_session(const std::string& path, std::vector<Message>& msgs) {
    std::ifstream f(path);
    if (!f) return false;
    msgs.clear();  // always start fresh — caller should not rely on pre-existing content
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        Message msg{};
        if (!parse_session_line(line, msg))
            return false;
        msgs.push_back(std::move(msg));
    }
    return true;
}

} // namespace t81::tui
