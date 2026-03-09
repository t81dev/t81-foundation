// SPDX-License-Identifier: MIT
// T81 TUI — shared utility implementations
#include "tooling/tui/common.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cwctype>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#  define NOMINMAX
#  include <windows.h>
#else
#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace t81::tui {

namespace fs = std::filesystem;

namespace {

std::string& cli_program_storage() {
    static std::string path = "t81";
    return path;
}

} // namespace

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

std::string compact_path(const std::string& path, size_t width) {
    if (path.size() <= width)
        return path;
    if (width <= 3)
        return path.substr(path.size() - width);
    return "..." + path.substr(path.size() - (width - 3));
}

TargetKind infer_target_kind(const std::string& path) {
    std::error_code ec;
    if (fs::is_directory(fs::path(path), ec))
        return TargetKind::Directory;
    const std::string ext = fs::path(path).extension().string();
    if (ext == ".t81" || ext == ".t81w")
        return TargetKind::SourceFile;
    if (ext == ".tisc")
        return TargetKind::ArtifactFile;
    if (ext == ".trace" || ext == ".log" || ext == ".txt")
        return TargetKind::TraceFile;
    if (ext == ".apl" || ext == ".axionb")
        return TargetKind::PolicyFile;
    return TargetKind::None;
}

std::string target_kind_label(TargetKind kind) {
    switch (kind) {
    case TargetKind::SourceFile: return "Source";
    case TargetKind::ArtifactFile: return "Artifact";
    case TargetKind::TraceFile: return "Trace";
    case TargetKind::PolicyFile: return "Policy";
    case TargetKind::Directory: return "Directory";
    case TargetKind::CanonFsObject: return "CanonFS";
    case TargetKind::None: return "None";
    }
    return "None";
}

TargetRef make_target_from_path(const std::string& path) {
    TargetRef target;
    target.kind = infer_target_kind(path);
    target.path = path;
    target.label = fs::path(path).filename().string();
    if (target.label.empty())
        target.label = path;
    return target;
}

std::string format_target_summary(const TargetRef& target) {
    if (target.kind == TargetKind::None)
        return "Current target: (none)";
    return target_kind_label(target.kind) + ": " + target.path;
}

void remember_target(std::vector<TargetRef>& recent_targets, TargetRef& current_target,
                     TargetRef& current_source, TargetRef& current_artifact,
                     TargetRef& current_trace, TargetRef& current_policy,
                     const TargetRef& target, size_t max_recent) {
    if (target.kind == TargetKind::None)
        return;
    current_target = target;
    if (target.kind == TargetKind::SourceFile)
        current_source = target;
    else if (target.kind == TargetKind::ArtifactFile)
        current_artifact = target;
    else if (target.kind == TargetKind::TraceFile)
        current_trace = target;
    else if (target.kind == TargetKind::PolicyFile)
        current_policy = target;
    recent_targets.erase(
        std::remove_if(recent_targets.begin(), recent_targets.end(),
            [&](const TargetRef& other) {
                return other.kind == target.kind &&
                       other.path == target.path &&
                       other.canon_hash == target.canon_hash;
            }),
        recent_targets.end());
    recent_targets.insert(recent_targets.begin(), target);
    if (recent_targets.size() > max_recent)
        recent_targets.resize(max_recent);
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
    auto utf8_to_wide = [](const std::string& s) -> std::wstring {
        if (s.empty()) return {};
        const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        if (len <= 0) return {};
        std::wstring out(static_cast<size_t>(len), L'\0');
        (void)MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
        out.pop_back();
        return out;
    };

    auto quote_windows_arg = [](const std::wstring& arg) -> std::wstring {
        if (arg.empty())
            return L"\"\"";
        bool needs_quotes = false;
        for (wchar_t c : arg) {
            if (std::iswspace(c) || c == L'"') {
                needs_quotes = true;
                break;
            }
        }
        if (!needs_quotes)
            return arg;

        std::wstring out = L"\"";
        int backslashes = 0;
        for (wchar_t c : arg) {
            if (c == L'\\') {
                ++backslashes;
                continue;
            }
            if (c == L'"') {
                out.append(static_cast<size_t>(backslashes * 2 + 1), L'\\');
                out += L'"';
                backslashes = 0;
                continue;
            }
            if (backslashes > 0) {
                out.append(static_cast<size_t>(backslashes), L'\\');
                backslashes = 0;
            }
            out += c;
        }
        if (backslashes > 0)
            out.append(static_cast<size_t>(backslashes * 2), L'\\');
        out += L'"';
        return out;
    };

    std::wstring cmdline;
    for (const auto& arg : argv) {
        if (!cmdline.empty())
            cmdline += L' ';
        cmdline += quote_windows_arg(utf8_to_wide(arg));
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0))
        return "[error: pipe failed]";
    (void)SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;

    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mutable_cmd(cmdline.begin(), cmdline.end());
    mutable_cmd.push_back(L'\0');
    const BOOL ok = CreateProcessW(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(write_pipe);
    if (!ok) {
        CloseHandle(read_pipe);
        const DWORD err = GetLastError();
        return "[error: CreateProcessW failed " + std::to_string(err) + "]";
    }

    std::string result;
    char buf[512];
    DWORD nread = 0;
    while (ReadFile(read_pipe, buf, sizeof(buf), &nread, nullptr) && nread > 0)
        result.append(buf, static_cast<size_t>(nread));
    CloseHandle(read_pipe);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD status = 0;
    (void)GetExitCodeProcess(pi.hProcess, &status);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return finalize_process_output(std::move(result), static_cast<int>(status));
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

void set_cli_program_path(std::string path) {
    if (!path.empty())
        cli_program_storage() = std::move(path);
}

std::string cli_program_path() {
    return cli_program_storage();
}

std::vector<std::string> t81_cli_argv(std::initializer_list<std::string> tail) {
    std::vector<std::string> argv;
    argv.reserve(tail.size() + 1);
    argv.push_back(cli_program_path());
    argv.insert(argv.end(), tail.begin(), tail.end());
    return argv;
}

std::vector<std::string> t81_cli_argv(const std::vector<std::string>& tail) {
    std::vector<std::string> argv;
    argv.reserve(tail.size() + 1);
    argv.push_back(cli_program_path());
    argv.insert(argv.end(), tail.begin(), tail.end());
    return argv;
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

static std::string target_kind_str(TargetKind kind) {
    switch (kind) {
    case TargetKind::SourceFile: return "source";
    case TargetKind::ArtifactFile: return "artifact";
    case TargetKind::TraceFile: return "trace";
    case TargetKind::PolicyFile: return "policy";
    case TargetKind::Directory: return "directory";
    case TargetKind::CanonFsObject: return "canonfs";
    case TargetKind::None: return "none";
    }
    return "none";
}

static TargetKind parse_target_kind(const std::string& s) {
    if (s == "source") return TargetKind::SourceFile;
    if (s == "artifact") return TargetKind::ArtifactFile;
    if (s == "trace") return TargetKind::TraceFile;
    if (s == "policy") return TargetKind::PolicyFile;
    if (s == "directory") return TargetKind::Directory;
    if (s == "canonfs") return TargetKind::CanonFsObject;
    return TargetKind::None;
}

std::string default_workspace_state_path() {
    const fs::path home = std::getenv("HOME") ? std::getenv("HOME") : ".";
    const fs::path dir = home / ".t81";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return ec ? std::string{} : (dir / "workspace.jsonl").string();
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

bool parse_json_object_fields(
    const std::string& line, std::vector<std::pair<std::string, std::string>>& fields)
{
    size_t pos = 0;
    skip_ws(line, pos);
    if (pos >= line.size() || line[pos] != '{')
        return false;
    ++pos;
    fields.clear();

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
        fields.push_back({std::move(key), std::move(value)});

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
    return pos == line.size();
}

TargetRef target_from_fields(const std::vector<std::pair<std::string, std::string>>& fields) {
    TargetRef target;
    for (const auto& [key, value] : fields) {
        if (key == "kind")
            target.kind = parse_target_kind(value);
        else if (key == "path")
            target.path = value;
        else if (key == "label")
            target.label = value;
        else if (key == "canon_hash")
            target.canon_hash = value;
    }
    return target;
}

} // namespace

bool load_session(const std::string& path, std::vector<Message>& msgs) {
    std::ifstream f(path);
    if (!f) return false;
    std::vector<Message> parsed;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        Message msg{};
        if (!parse_session_line(line, msg))
            return false;
        parsed.push_back(std::move(msg));
    }
    msgs = std::move(parsed);
    return true;
}

bool save_workspace_state(const std::string& path, const WorkspaceState& state) {
    std::ofstream f(path);
    if (!f) return false;

    auto write_target = [&](const std::string& entry, const TargetRef& target) {
        f << "{\"entry\":\"" << entry
          << "\",\"kind\":\"" << target_kind_str(target.kind)
          << "\",\"path\":\"" << escape_json_string(target.path)
          << "\",\"label\":\"" << escape_json_string(target.label)
          << "\",\"canon_hash\":\"" << escape_json_string(target.canon_hash)
          << "\"}\n";
    };

    f << "{\"entry\":\"project_root\",\"path\":\""
      << escape_json_string(state.project_root) << "\"}\n";
    write_target("current_target", state.current_target);
    write_target("current_source", state.current_source);
    write_target("current_artifact", state.current_artifact);
    write_target("current_trace", state.current_trace);
    write_target("current_policy", state.current_policy);
    write_target("lhs_target", state.lhs_target);
    write_target("rhs_target", state.rhs_target);
    for (const auto& target : state.recent_targets)
        write_target("recent", target);
    return f.good();
}

bool load_workspace_state(const std::string& path, WorkspaceState& state) {
    std::ifstream f(path);
    if (!f) return false;
    WorkspaceState parsed;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::vector<std::pair<std::string, std::string>> fields;
        if (!parse_json_object_fields(line, fields))
            return false;
        std::string entry;
        std::string project_root;
        for (const auto& [key, value] : fields) {
            if (key == "entry")
                entry = value;
            else if (key == "path" && entry == "project_root")
                project_root = value;
        }
        if (entry == "project_root") {
            parsed.project_root = project_root;
            continue;
        }
        const TargetRef target = target_from_fields(fields);
        if (entry == "current_target")
            parsed.current_target = target;
        else if (entry == "current_source")
            parsed.current_source = target;
        else if (entry == "current_artifact")
            parsed.current_artifact = target;
        else if (entry == "current_trace")
            parsed.current_trace = target;
        else if (entry == "current_policy")
            parsed.current_policy = target;
        else if (entry == "lhs_target")
            parsed.lhs_target = target;
        else if (entry == "rhs_target")
            parsed.rhs_target = target;
        else if (entry == "recent")
            parsed.recent_targets.push_back(target);
    }
    state = std::move(parsed);
    return true;
}

} // namespace t81::tui
