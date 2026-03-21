// SPDX-License-Identifier: MIT
// T81 TUI — AI-Native / Agentic Interface (t81 agent)
#include "tools/tui/agent.hpp"
#include "tools/tui/common.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace ftxui;
namespace fs = std::filesystem;

namespace t81::tui {

static bool line_looks_like_error(const std::string& text) {
    std::string lower = text;
    for (char& c : lower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return lower.find("[error:") != std::string::npos ||
           lower.find("[exit status ") != std::string::npos ||
           lower.find("permission denied") != std::string::npos ||
           lower.find("not found") != std::string::npos;
}

// ── Helpers ────────────────────────────────────────────────────────────────

// Generate ~/.t81/sessions/<YYYY-MM-DDTHH-MM-SS>.jsonl
static std::string default_session_path() {
    const fs::path dir = fs::path(std::getenv("HOME") ? std::getenv("HOME") : ".")
                         / ".t81" / "sessions";
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) return "";
    auto now  = std::chrono::system_clock::now();
    auto tt   = std::chrono::system_clock::to_time_t(now);
    std::tm  tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H-%M-%S");
    return (dir / (oss.str() + ".jsonl")).string();
}

// Extract a single-line value from command output by scanning for a keyword.
static std::string extract_token(const std::string& out, const std::string& keyword) {
    auto pos = out.find(keyword);
    if (pos == std::string::npos) return {};
    pos += keyword.size();
    while (pos < out.size() && (out[pos] == ' ' || out[pos] == ':' || out[pos] == '='))
        ++pos;
    size_t end = pos;
    while (end < out.size() && out[end] != '\n' && out[end] != '\r' && out[end] != ' ')
        ++end;
    return out.substr(pos, end - pos);
}

// Update SessionState from the output of axion status.
static void update_axion_state(SessionState& state, const std::string& out) {
    // Look for mode keywords
    if (out.find("Strict")  != std::string::npos) state.axion_mode = "Strict";
    else if (out.find("Audit")   != std::string::npos) state.axion_mode = "Audit";
    else if (out.find("Permissive") != std::string::npos) state.axion_mode = "Permissive";
    else if (out.find("Disabled") != std::string::npos) state.axion_mode = "Disabled";
}

// Update trace hash from determinism / trace output.
static void update_trace_state(SessionState& state, const std::string& out) {
    // `t81 determinism hash` prints "hash: <hex>" or just a bare hex string
    std::string h = extract_token(out, "hash");
    if (h.empty()) h = extract_token(out, "Hash");
    if (h.empty()) h = extract_token(out, "sha");
    if (!h.empty() && h.size() >= 8)
        state.trace_hash = h.substr(0, 12) + "…";
}

static std::vector<std::string> prepend_args(
    std::initializer_list<std::string> prefix, const std::vector<std::string>& tail)
{
    std::vector<std::string> argv(prefix);
    argv.insert(argv.end(), tail.begin(), tail.end());
    return argv;
}

static std::string compact_session_path(const std::string& path) {
    constexpr size_t kMax = 38;
    if (path.size() <= kMax)
        return path;
    return "..." + path.substr(path.size() - (kMax - 3));
}

static Element render_message_block(const std::string& prefix,
                                    Color prefix_color,
                                    const std::string& body,
                                    Color body_color) {
    Elements rows;
    const auto lines = split_lines(body);
    for (size_t i = 0; i < lines.size(); ++i) {
        rows.push_back(hbox({
            text(i == 0 ? prefix : std::string(prefix.size(), ' '))
                | bold | color(prefix_color),
            paragraph(lines[i]) | color(body_color),
        }));
    }
    if (rows.empty()) {
        rows.push_back(hbox({
            text(prefix) | bold | color(prefix_color),
            text(""),
        }));
    }
    return vbox(std::move(rows));
}

// ── Slash-command dispatch ─────────────────────────────────────────────────

static std::string handle_slash(
    const std::string& cmd_line,
    SessionState&      state,
    const std::string& session_path,
    const std::vector<Message>& history,
    TargetRef&         current_target,
    TargetRef&         current_source,
    TargetRef&         current_artifact,
    TargetRef&         current_trace,
    TargetRef&         current_policy,
    TargetRef&         lhs_target,
    TargetRef&         rhs_target,
    std::vector<TargetRef>& recent_targets)
{
    std::istringstream ss(cmd_line.substr(1));
    std::string verb;
    ss >> verb;
    std::string rest;
    std::getline(ss, rest);
    if (!rest.empty() && rest.front() == ' ') rest.erase(0, 1);

    // ── Help ────────────────────────────────────────────────────────────────
    if (verb == "help") {
        return
            "Slash commands:\n"
            "  /compile <file>                  Compile .t81 → TISC (updates trace hash)\n"
            "  /run [file]                      Compile and execute current or named target\n"
            "  /check [file]                    Syntax-check current or named source\n"
            "  /disasm [file]                   Disassemble current or named TISC bytecode\n"
            "  /trace [file]                    Show current or named trace file\n"
            "  /hash [file]                     Compute determinism hash for current/named target\n"
            "  /infer <model> --policy <p> <q>  Run governed llama-run inference\n"
            "                                   (updates trit-probability display)\n"
            "  /axion                           Show Axion policy status (updates context)\n"
            "  /policy <action>                 Policy: list | validate <f> | status\n"
            "  /allow <hash>                    Append hash to active Axion policy whitelist\n"
            "  /open <file>                     Set the current target\n"
            "  /current                         Show the current target\n"
            "  /recent                          Show recent targets\n"
            "  /use lhs <file>                  Assign compare lhs target\n"
            "  /use rhs <file>                  Assign compare rhs target\n"
            "  /write <file> <content>          Write content to file on disk\n"
            "  /tier <n>                        Set cognitive tier display (1-5)\n"
            "  /trits                           Toggle trit-probability display\n"
            "  /save                            Save session to disk\n"
            "  /clear                           Clear conversation history\n"
            "  /quit                            Exit\n"
            "\n"
            "Keyboard shortcuts:\n"
            "  PgUp / PgDn             Scroll conversation history\n"
            "  F2 / Ctrl+O             Toggle context panel\n"
            "  Escape                  Exit\n";
    }

    if (verb == "open" && !rest.empty()) {
        TargetRef target = make_target_from_path(rest);
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy, target, 8);
        return "Selected " + rest + ".";
    }
    if (verb == "current") {
        std::string out = t81::tui::format_target_summary(current_target);
        if (lhs_target.kind != TargetKind::None)
            out += "\nLHS: " + lhs_target.path;
        if (rhs_target.kind != TargetKind::None)
            out += "\nRHS: " + rhs_target.path;
        return out;
    }
    if (verb == "recent") {
        if (recent_targets.empty())
            return "No recent targets.";
        std::string out = "Recent targets:\n";
        for (size_t i = 0; i < recent_targets.size(); ++i)
            out += "  " + std::to_string(i + 1) + ". " + recent_targets[i].path + "\n";
        if (!out.empty() && out.back() == '\n')
            out.pop_back();
        return out;
    }
    if (verb == "use") {
        std::istringstream rs(rest);
        std::string slot;
        rs >> slot;
        std::string path;
        std::getline(rs, path);
        if (!path.empty() && path.front() == ' ')
            path.erase(0, 1);
        if ((slot != "lhs" && slot != "rhs") || path.empty())
            return "[/use requires lhs|rhs <file>]";
        TargetRef target = make_target_from_path(path);
        if (slot == "lhs") {
            lhs_target = target;
            return "LHS set to " + path;
        }
        rhs_target = target;
        return "RHS set to " + path;
    }

    // ── Code lifecycle ───────────────────────────────────────────────────────
    if (verb == "compile") {
        const std::string path = !rest.empty() ? rest : current_source.path;
        if (path.empty())
            return "[/compile requires <file> or a current source target]";
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  make_target_from_path(path), 8);
        const std::string out = exec_argv(
            t81_cli_argv(prepend_args({"code", "build"}, split_command_words(path))));
        update_trace_state(state, out);
        if (const auto built = extract_token(out, "wrote"); !built.empty()) {
            TargetRef artifact = make_target_from_path(built);
            t81::tui::remember_target(recent_targets, current_target, current_source,
                                      current_artifact, current_trace, current_policy, artifact, 8);
        }
        return out;
    }
    if (verb == "run") {
        const std::string path = !rest.empty()
            ? rest
            : (!current_artifact.path.empty() ? current_artifact.path : current_source.path);
        if (path.empty())
            return "[/run requires <file> or a current source/artifact target]";
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  make_target_from_path(path), 8);
        const std::string out = exec_argv(
            t81_cli_argv(prepend_args({"code", "run"}, split_command_words(path))));
        update_trace_state(state, out);
        return out;
    }
    if (verb == "check") {
        const std::string path = !rest.empty() ? rest : current_source.path;
        if (path.empty())
            return "[/check requires <file> or a current source target]";
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  make_target_from_path(path), 8);
        return exec_argv(t81_cli_argv(prepend_args({"code", "check"}, split_command_words(path))));
    }
    if (verb == "disasm") {
        const std::string path = !rest.empty() ? rest : current_artifact.path;
        if (path.empty())
            return "[/disasm requires <file> or a current artifact target]";
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  make_target_from_path(path), 8);
        return exec_argv(t81_cli_argv(prepend_args({"code", "disasm"}, split_command_words(path))));
    }

    // ── Inference (/infer <model> [--policy <p>] <prompt>) ───────────────────
    if (verb == "infer") {
        if (rest.empty())
            return "[/infer requires: <model.gguf|sha3-256:hash> --policy <p> <prompt>]";
        const auto infer_args = split_command_words(rest);
        if (infer_args.empty())
            return "[/infer requires: <model.gguf|sha3-256:hash> --policy <p> <prompt>]";
        const std::string out =
            exec_argv(t81_cli_argv(prepend_args({"internal", "llama-run"}, infer_args)));
        // Parse model name from the first positional token
        {
            const std::string& m = infer_args.front();
            if (!m.empty()) {
                const auto slash = m.rfind('/');
                state.model_name = (slash != std::string::npos) ? m.substr(slash + 1) : m;
            }
        }
        // Update trace hash if provided
        update_trace_state(state, out);
        // Parse token_ids_csv → trit distribution
        // Each token_id % 3: 0→P(0), 1→P(+1), 2→P(-1)
        {
            const std::string needle = "token_ids_csv:";
            auto pos = out.find(needle);
            if (pos != std::string::npos) {
                pos += needle.size();
                size_t end = out.find('\n', pos);
                const std::string csv = out.substr(pos,
                    end == std::string::npos ? std::string::npos : end - pos);
                int cnt_pos = 0, cnt_zero = 0, cnt_neg = 0, total = 0;
                std::istringstream ts(csv);
                std::string tok;
                while (std::getline(ts, tok, ',')) {
                    try {
                        const int id = std::stoi(tok);
                        const int r  = ((id % 3) + 3) % 3;
                        if (r == 1)      ++cnt_pos;
                        else if (r == 0) ++cnt_zero;
                        else             ++cnt_neg;
                        ++total;
                    } catch (...) {}
                }
                if (total > 0) {
                    state.trit_pos   = static_cast<float>(cnt_pos)  / total;
                    state.trit_zero  = static_cast<float>(cnt_zero) / total;
                    state.trit_neg   = static_cast<float>(cnt_neg)  / total;
                    state.infer_tokens = total;
                }
            }
        }
        return out;
    }

    // ── Determinism & tracing ────────────────────────────────────────────────
    if (verb == "trace") {
        const std::string path = !rest.empty() ? rest : current_trace.path;
        if (path.empty())
            return "[/trace requires <file> or a current trace target]";
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  make_target_from_path(path), 8);
        const std::string out = exec_argv(
            t81_cli_argv(prepend_args({"trace", "show"}, split_command_words(path))));
        update_trace_state(state, out);
        return out;
    }
    if (verb == "hash") {
        const std::string path = !rest.empty() ? rest : current_target.path;
        if (path.empty())
            return "[/hash requires <file> or a current target]";
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  make_target_from_path(path), 8);
        const std::string out = exec_argv(
            t81_cli_argv(prepend_args({"determinism", "hash"}, split_command_words(path))));
        update_trace_state(state, out);
        return out;
    }

    // ── Axion / policy ───────────────────────────────────────────────────────
    if (verb == "axion") {
        const std::string out = exec_argv(t81_cli_argv({"axion", "status"}));
        update_axion_state(state, out);
        return out;
    }
    if (verb == "policy") {
        if (rest.empty()) rest = "list";
        const std::string out = exec_argv(
            t81_cli_argv(prepend_args({"policy"}, split_command_words(rest))));
        // If it was a status query, update axion state too
        if (rest == "status") update_axion_state(state, out);
        return out;
    }
    if (verb == "allow" && !rest.empty()) {
        // Append the hash to the active policy's allowed-tensor-hashes list.
        // Delegates to the CLI which knows the policy file location.
        const std::string out = exec_argv(
            t81_cli_argv(prepend_args({"policy", "allow-hash"}, split_command_words(rest))));
        if (out.find("error") == std::string::npos &&
            out.find("Error") == std::string::npos)
            return "Hash " + rest.substr(0, 16) + "… added to allowed-tensor-hashes.\n" + out;
        return out;
    }

    // ── File writing (/write <path> <content>) ───────────────────────────────
    if (verb == "write") {
        const auto sp = rest.find(' ');
        if (sp == std::string::npos)
            return "[/write requires <file> <content>]";
        const std::string path    = rest.substr(0, sp);
        const std::string content = rest.substr(sp + 1);
        std::ofstream f(path);
        if (!f) return "[error: could not open " + path + " for writing]";
        f << content;
        if (!f) return "[error: write failed for " + path + "]";
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  make_target_from_path(path), 8);
        return "Written " + std::to_string(content.size()) +
               " bytes to " + path + ".";
    }

    // ── State mutations ──────────────────────────────────────────────────────
    if (verb == "tier" && !rest.empty()) {
        try {
            int t = std::stoi(rest);
            if (t >= 1 && t <= 5) {
                state.vm_tier = t;
                return "Cognitive tier set to " + rest + ".";
            }
        } catch (...) {}
        return "[/tier requires an integer 1-5]";
    }
    if (verb == "save") {
        if (session_path.empty())
            return "No session path set. Launch with --session <path>, or sessions "
                   "are auto-saved to ~/.t81/sessions/ when no flag is given.";
        return save_session(session_path, history)
            ? "Session saved to " + session_path + "."
            : "[error: failed to save session]";
    }

    return "[unknown command /" + verb + " — try /help]";
}

// ── Trit probability bar ───────────────────────────────────────────────────
static Element trit_bar(float p_pos, float p_zero, float p_neg) {
    p_pos  = std::max(0.f, std::min(1.f, p_pos));
    p_zero = std::max(0.f, std::min(1.f, p_zero));
    p_neg  = std::max(0.f, std::min(1.f, p_neg));
    const int width = 16;
    auto bar = [&](float p, Color c) -> Element {
        int filled = static_cast<int>(p * width);
        std::string s(static_cast<size_t>(filled), '#');
        s += std::string(static_cast<size_t>(width - filled), '-');
        return text(s) | color(c);
    };
    return vbox({
        hbox({ text("+1 ") | color(Color::Green),  bar(p_pos,  Color::Green)  }),
        hbox({ text(" 0 ") | color(Color::Yellow), bar(p_zero, Color::Yellow) }),
        hbox({ text("-1 ") | color(Color::Red),    bar(p_neg,  Color::Red)    }),
    });
}

// ── run_agent ──────────────────────────────────────────────────────────────

int run_agent(const std::vector<std::string>& args) {
    // ── Parse flags ──────────────────────────────────────────────────────────
    std::string resume_path;
    std::string session_path;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i + 1 < args.size()) {
            if (args[i] == "--resume")  { resume_path  = args[++i]; continue; }
            if (args[i] == "--session") { session_path = args[++i]; continue; }
        }
    }

    // Default session path if none given
    if (session_path.empty()) session_path = default_session_path();

    // ── State ────────────────────────────────────────────────────────────────
    SessionState         state;
    std::mutex           state_mutex;
    TargetRef            current_target;
    TargetRef            current_source;
    TargetRef            current_artifact;
    TargetRef            current_trace;
    TargetRef            current_policy;
    TargetRef            lhs_target;
    TargetRef            rhs_target;
    std::vector<TargetRef> recent_targets;
    std::mutex           target_mutex;
    std::vector<Message> history;
    std::mutex           history_mutex;
    const std::string    workspace_path = default_workspace_state_path();
    std::string          workspace_project_root;
    bool                 show_trits     = false;
    bool                 quit_requested = false;
    bool                 show_context   = true;
    bool                 target_overlay_open = false;
    int                  target_overlay_selected = 0;
    int                  pending_jobs   = 0;
    bool                 resume_loaded  = resume_path.empty();
    std::string          busy_status;
    std::string          last_error;
    {
        WorkspaceState workspace;
        if (!workspace_path.empty() && load_workspace_state(workspace_path, workspace)) {
            workspace_project_root = workspace.project_root;
            current_target = workspace.current_target;
            current_source = workspace.current_source;
            current_artifact = workspace.current_artifact;
            current_trace = workspace.current_trace;
            current_policy = workspace.current_policy;
            lhs_target = workspace.lhs_target;
            rhs_target = workspace.rhs_target;
            recent_targets = workspace.recent_targets;
        }
    }

    if (!resume_path.empty()) {
        if (!load_session(resume_path, history)) {
            history.clear();
            history.push_back({Message::Role::System,
                "[error: failed to resume session from " + resume_path + "]"});
            last_error = "resume failed";
            resume_loaded = false;
        } else {
            resume_loaded = true;
        }
    }

    // Welcome / resume message
    {
        std::lock_guard<std::mutex> lk(history_mutex);
        history.push_back({Message::Role::System,
            (!resume_path.empty() && resume_loaded)
                ? "Session resumed from " + resume_path + ". Type /help for commands."
                : "T81 Agentic Interface. Type /help for commands."});
    }

    std::string input_buf;

    // ── FTXUI setup ──────────────────────────────────────────────────────────
    auto screen  = ScreenInteractive::Fullscreen();
    auto input_c = Input(&input_buf, "Type a message or /command…");
    std::mutex task_mutex;
    std::vector<std::function<void()>> pending_ui_tasks;

    auto history_log_viewer = LogViewer([&]() {
        Elements msgs;
        std::vector<Message> history_snapshot;
        {
            std::lock_guard<std::mutex> lk(history_mutex);
            history_snapshot = history;
        }
        for (const auto& m : history_snapshot) {
            switch (m.role) {
            case Message::Role::User:
                msgs.push_back(render_message_block(
                    "[You]   ", Color::Green, m.text, Color::White));
                break;
            case Message::Role::Agent:
                msgs.push_back(render_message_block(
                    "[Agent] ", Color::Cyan, m.text,
                    line_looks_like_error(m.text) ? Color::Red : Color::White));
                break;
            case Message::Role::System:
                msgs.push_back(
                    text("* " + m.text)
                        | color(line_looks_like_error(m.text)
                            ? Color::Red
                            : Color::GrayDark)
                        | dim
                );
                break;
            }
            msgs.push_back(text(""));
        }
        return msgs;
    });

    auto queue_ui = [&](std::function<void()> fn) {
        {
            std::lock_guard<std::mutex> lk(task_mutex);
            pending_ui_tasks.push_back(std::move(fn));
        }
        screen.PostEvent(Event::Custom);
    };

    auto set_error = [&](const std::string& text) {
        if (line_looks_like_error(text))
            last_error = text.substr(0, 80);
    };
    auto save_workspace = [&]() {
        if (workspace_path.empty())
            return;
        WorkspaceState workspace;
        workspace.project_root = workspace_project_root;
        workspace.current_target = current_target;
        workspace.current_source = current_source;
        workspace.current_artifact = current_artifact;
        workspace.current_trace = current_trace;
        workspace.current_policy = current_policy;
        workspace.lhs_target = lhs_target;
        workspace.rhs_target = rhs_target;
        workspace.recent_targets = recent_targets;
        (void)save_workspace_state(workspace_path, workspace);
    };
    auto save_workspace_snapshot = [&](const TargetRef& current_target_value,
                                       const TargetRef& current_source_value,
                                       const TargetRef& current_artifact_value,
                                       const TargetRef& current_trace_value,
                                       const TargetRef& current_policy_value,
                                       const TargetRef& lhs_target_value,
                                       const TargetRef& rhs_target_value,
                                       const std::vector<TargetRef>& recent_targets_value) {
        if (workspace_path.empty())
            return;
        WorkspaceState workspace;
        workspace.project_root = workspace_project_root;
        workspace.current_target = current_target_value;
        workspace.current_source = current_source_value;
        workspace.current_artifact = current_artifact_value;
        workspace.current_trace = current_trace_value;
        workspace.current_policy = current_policy_value;
        workspace.lhs_target = lhs_target_value;
        workspace.rhs_target = rhs_target_value;
        workspace.recent_targets = recent_targets_value;
        (void)save_workspace_state(workspace_path, workspace);
    };

    ++pending_jobs;
    busy_status = "Refreshing axion status";
    std::thread([&]() {
        const std::string ax = exec_argv(t81_cli_argv({"axion", "status"}));
        queue_ui([&, ax]() {
            {
                std::lock_guard<std::mutex> lk(state_mutex);
                update_axion_state(state, ax);
            }
            set_error(ax);
            pending_jobs = std::max(0, pending_jobs - 1);
            if (pending_jobs == 0)
                busy_status.clear();
        });
    }).detach();

    input_c = CatchEvent(input_c, [&](Event e) -> bool {
        if (e != Event::Return || input_buf.empty()) return false;
        const std::string text_in = input_buf;
        input_buf.clear();

        // Early exits that don't push to history
        if (text_in == "/quit" || text_in == "/q") {
            quit_requested = true;
            screen.ExitLoopClosure()();
            return true;
        }
        if (text_in == "/clear") {
            std::lock_guard<std::mutex> lk(history_mutex);
            history.clear();
            history.push_back({Message::Role::System,
                "Conversation cleared. Type /help for commands."});
            return true;
        }
        if (text_in == "/trits") {
            show_trits = !show_trits;
            std::lock_guard<std::mutex> lk(history_mutex);
            history.push_back({Message::Role::System,
                std::string("Trit probability display ")
                + (show_trits ? "ON." : "OFF.")});
            return true;
        }

        // Record user turn
        {
            std::lock_guard<std::mutex> lk(history_mutex);
            history.push_back({Message::Role::User, text_in});
        }

        // Dispatch
        if (!text_in.empty() && text_in.front() == '/') {
            auto slash_verb = [&]() {
                std::istringstream ss(text_in.substr(1));
                std::string verb;
                ss >> verb;
                return verb;
            }();
            const bool local_only =
                slash_verb == "open" || slash_verb == "current" ||
                slash_verb == "recent" || slash_verb == "use" ||
                slash_verb == "tier" ||
                slash_verb == "save";
            std::vector<Message> history_snapshot;
            SessionState state_snapshot;
            TargetRef current_target_snapshot;
            TargetRef current_source_snapshot;
            TargetRef current_artifact_snapshot;
            TargetRef current_trace_snapshot;
            TargetRef current_policy_snapshot;
            TargetRef lhs_target_snapshot;
            TargetRef rhs_target_snapshot;
            std::vector<TargetRef> recent_targets_snapshot;
            {
                std::lock_guard<std::mutex> lk(history_mutex);
                history_snapshot = history;
            }
            {
                std::lock_guard<std::mutex> lk(state_mutex);
                state_snapshot = state;
            }
            {
                std::lock_guard<std::mutex> lk(target_mutex);
                current_target_snapshot = current_target;
                current_source_snapshot = current_source;
                current_artifact_snapshot = current_artifact;
                current_trace_snapshot = current_trace;
                current_policy_snapshot = current_policy;
                lhs_target_snapshot = lhs_target;
                rhs_target_snapshot = rhs_target;
                recent_targets_snapshot = recent_targets;
            }
            if (local_only) {
                std::string reply =
                    handle_slash(text_in, state_snapshot, session_path, history_snapshot,
                                 current_target_snapshot, current_source_snapshot,
                                 current_artifact_snapshot, current_trace_snapshot,
                                 current_policy_snapshot, lhs_target_snapshot,
                                 rhs_target_snapshot, recent_targets_snapshot);
                {
                    std::lock_guard<std::mutex> lk(state_mutex);
                    state = state_snapshot;
                }
                {
                    std::lock_guard<std::mutex> lk(target_mutex);
                    current_target = current_target_snapshot;
                    current_source = current_source_snapshot;
                    current_artifact = current_artifact_snapshot;
                    current_trace = current_trace_snapshot;
                    current_policy = current_policy_snapshot;
                    lhs_target = lhs_target_snapshot;
                    rhs_target = rhs_target_snapshot;
                    recent_targets = std::move(recent_targets_snapshot);
                    save_workspace_snapshot(current_target, current_source,
                        current_artifact, current_trace, current_policy,
                        lhs_target, rhs_target, recent_targets);
                }
                set_error(reply);
                {
                    std::lock_guard<std::mutex> lk(history_mutex);
                    history.push_back({Message::Role::Agent, reply});
                }
                if (!session_path.empty()) {
                    std::vector<Message> snapshot;
                    {
                        std::lock_guard<std::mutex> lk(history_mutex);
                        snapshot = history;
                    }
                    if (!save_session(session_path, snapshot)) {
                        std::lock_guard<std::mutex> lk(history_mutex);
                        history.push_back({Message::Role::System,
                            "[error: failed to save session]"});
                        last_error = "save failed";
                    }
                }
                return true;
            }
            ++pending_jobs;
            busy_status = "Running " + text_in.substr(1);
            screen.PostEvent(Event::Custom);
            std::thread([&, text_in, history_snapshot = std::move(history_snapshot),
                         state_snapshot,
                         current_target_snapshot,
                         current_source_snapshot,
                         current_artifact_snapshot,
                         current_trace_snapshot,
                         current_policy_snapshot,
                         lhs_target_snapshot,
                         rhs_target_snapshot,
                         recent_targets_snapshot = std::move(recent_targets_snapshot)]() mutable {
                std::string reply =
                    handle_slash(text_in, state_snapshot, session_path, history_snapshot,
                                 current_target_snapshot, current_source_snapshot,
                                 current_artifact_snapshot, current_trace_snapshot,
                                 current_policy_snapshot, lhs_target_snapshot,
                                 rhs_target_snapshot, recent_targets_snapshot);
                queue_ui([&, reply = std::move(reply), state_snapshot,
                          current_target_snapshot, current_source_snapshot,
                          current_artifact_snapshot, current_trace_snapshot,
                          current_policy_snapshot,
                          lhs_target_snapshot,
                          rhs_target_snapshot,
                          recent_targets_snapshot = std::move(recent_targets_snapshot)]() mutable {
                    {
                        std::lock_guard<std::mutex> lk(state_mutex);
                        state = state_snapshot;
                    }
                    {
                        std::lock_guard<std::mutex> lk(target_mutex);
                        current_target = current_target_snapshot;
                        current_source = current_source_snapshot;
                        current_artifact = current_artifact_snapshot;
                        current_trace = current_trace_snapshot;
                        current_policy = current_policy_snapshot;
                        lhs_target = lhs_target_snapshot;
                        rhs_target = rhs_target_snapshot;
                        recent_targets = std::move(recent_targets_snapshot);
                        save_workspace_snapshot(current_target, current_source,
                            current_artifact, current_trace, current_policy,
                            lhs_target, rhs_target, recent_targets);
                    }
                    set_error(reply);
                    {
                        std::lock_guard<std::mutex> lk(history_mutex);
                        history.push_back({Message::Role::Agent, reply});
                    }
                    pending_jobs = std::max(0, pending_jobs - 1);
                    if (pending_jobs == 0)
                        busy_status.clear();

                    if (!session_path.empty()) {
                        std::vector<Message> snapshot;
                        {
                            std::lock_guard<std::mutex> lk(history_mutex);
                            snapshot = history;
                        }
                        if (!save_session(session_path, snapshot)) {
                            std::lock_guard<std::mutex> lk(history_mutex);
                            history.push_back({Message::Role::System,
                                "[error: failed to save session]"});
                            last_error = "save failed";
                        }
                    }
                });
            }).detach();
        } else {
            {
                std::lock_guard<std::mutex> lk(history_mutex);
                history.push_back({Message::Role::Agent,
                    "[Agent] Received: \"" + text_in + "\"\n"
                    "Use slash commands to interact with T81 subsystems:\n"
                    "  /compile /run /check /disasm /trace /hash\n"
                    "  /axion /policy /allow /write /tier /trits /help"});
            }
            if (!session_path.empty()) {
                std::vector<Message> snapshot;
                {
                    std::lock_guard<std::mutex> lk(history_mutex);
                    snapshot = history;
                }
                if (!save_session(session_path, snapshot)) {
                    std::lock_guard<std::mutex> lk(history_mutex);
                    history.push_back({Message::Role::System,
                        "[error: failed to save session]"});
                    last_error = "save failed";
                }
            }
        }
        return true;
    });

    auto layout = Container::Vertical({ history_log_viewer, input_c });

    // ── Renderer ─────────────────────────────────────────────────────────────
    auto renderer = Renderer(layout, [&]() -> Element {
        SessionState state_snapshot;
        TargetRef current_target_snapshot;
        TargetRef lhs_target_snapshot;
        TargetRef rhs_target_snapshot;
        std::vector<TargetRef> recent_targets_snapshot;
        {
            std::lock_guard<std::mutex> lk(state_mutex);
            state_snapshot = state;
        }
        {
            std::lock_guard<std::mutex> lk(target_mutex);
            current_target_snapshot = current_target;
            lhs_target_snapshot = lhs_target;
            rhs_target_snapshot = rhs_target;
            recent_targets_snapshot = recent_targets;
        }
        // Status bar
        auto status = hbox({
            !last_error.empty()
                ? text(" ✗ System Log: " + last_error) | color(Color::Red)
                : text(" ✓ System Log: OK") | color(Color::GrayDark),
            filler(),
            pending_jobs > 0
                ? text(" Busy: " + busy_status + "  |  ") | color(Color::Yellow)
                : text(""),
            text("PgUp/PgDn: scroll   F2: context   F3: targets   /help   Esc: exit ")
                | color(Color::GrayDark),
        }) | bgcolor(Color::Black);

        // Context panel
        auto context_panel = vbox({
            text(" Context") | bold | color(Color::Cyan),
            separator(),
            text(" Model:  " + state_snapshot.model_name),
            text(" Tier:   " + std::to_string(state_snapshot.vm_tier)),
            hbox({ text(" Axion:  "),
                   text(state_snapshot.axion_mode)
                       | color(state_snapshot.axion_mode == "Strict"  ? Color::Green  :
                               state_snapshot.axion_mode == "Audit"   ? Color::Yellow :
                               state_snapshot.axion_mode == "Disabled"? Color::Red    :
                                                               Color::White) }),
            hbox({ text(" Trace:  "),
                   text(state_snapshot.trace_hash) | color(Color::Green) }),
            separator(),
            text(" Trit Probs:") | color(Color::GrayDark),
            show_trits
                ? (state_snapshot.infer_tokens > 0
                    ? vbox({
                        trit_bar(state_snapshot.trit_pos, state_snapshot.trit_zero,
                                 state_snapshot.trit_neg),
                        text(" n=" + std::to_string(state_snapshot.infer_tokens) + " tokens")
                            | color(Color::GrayDark),
                      })
                    : vbox({
                        trit_bar(state_snapshot.trit_pos, state_snapshot.trit_zero,
                                 state_snapshot.trit_neg),
                        text(" run /infer to populate") | color(Color::GrayDark),
                      }))
                : (text(" [/trits to enable]") | color(Color::GrayDark)),
            filler(),
            separator(),
            text(" Target:") | color(Color::GrayDark),
            text(" " + compact_path(current_target_snapshot.path.empty()
                    ? "(none)" : current_target_snapshot.path))
                | color(Color::Green),
            text(" LHS: " + compact_path(lhs_target_snapshot.path.empty()
                    ? "(none)" : lhs_target_snapshot.path, 20))
                | color(Color::GrayDark),
            text(" RHS: " + compact_path(rhs_target_snapshot.path.empty()
                    ? "(none)" : rhs_target_snapshot.path, 20))
                | color(Color::GrayDark),
            text(" Recent: " + std::to_string(recent_targets_snapshot.size()))
                | color(Color::GrayDark),
            separator(),
            separator(),
            text(" Session:") | color(Color::GrayDark),
            text(session_path.empty() ? "(none)" : compact_session_path(session_path))
                | color(Color::GrayDark),
        }) | border | size(WIDTH, LESS_THAN, 24);

        auto history_pane = vbox({
            text(" Interaction History") | bold | color(Color::White),
            separator(),
            history_log_viewer->Render() | flex,
            separator(),
            hbox({
                text(" >> ") | color(Color::Green),
                input_c->Render() | flex,
            }),
        }) | border | flex;

        auto top_bar = hbox({
            text(" T81 Agentic Interface") | bold | color(Color::Cyan),
            filler(),
            text("[Cmd: Ctrl+P / + /] ") | color(Color::GrayDark),
        });

        auto main = vbox({
            top_bar,
            show_context ? hbox({ history_pane, context_panel }) | flex : history_pane,
            status,
        });

        if (target_overlay_open) {
            const int n = static_cast<int>(recent_targets_snapshot.size());
            if (n > 0)
                target_overlay_selected = std::max(0, std::min(target_overlay_selected, n - 1));
            Elements rows;
            for (int i = 0; i < n; ++i) {
                auto row = hbox({
                    text("  " + std::to_string(i + 1) + ". ") | color(Color::GrayDark),
                    text(compact_path(recent_targets_snapshot[static_cast<size_t>(i)].path, 28)),
                    filler(),
                });
                rows.push_back(i == target_overlay_selected ? row | inverted : row);
            }
            if (rows.empty())
                rows.push_back(text("  No recent targets") | color(Color::GrayDark));
            auto overlay = vbox({
                text(" Recent Targets") | bold | color(Color::Cyan),
                separator(),
                vbox(std::move(rows)),
                separator(),
                text(" Enter: select   Esc/F3: close   ↑↓: move ") | color(Color::GrayDark),
            }) | border
              | size(WIDTH, EQUAL, 52)
              | size(HEIGHT, LESS_THAN, 14);

            return dbox({
                main,
                vbox({
                    filler(),
                    hbox({ filler(), overlay, filler() }),
                    filler(),
                }),
            });
        }

        return main;
    });

    // ── Global key handler ───────────────────────────────────────────────────
    auto root = CatchEvent(renderer, [&](Event e) -> bool {
        if (e == Event::Custom) {
            std::vector<std::function<void()>> tasks;
            {
                std::lock_guard<std::mutex> lk(task_mutex);
                tasks.swap(pending_ui_tasks);
            }
            for (auto& task : tasks)
                task();
            return true;
        }
        if (e == Event::Escape) {
            if (target_overlay_open) {
                target_overlay_open = false;
                return true;
            }
            screen.ExitLoopClosure()();
            return true;
        }
        if (e == Event::F3) {
            target_overlay_open = !target_overlay_open;
            target_overlay_selected = 0;
            return true;
        }
        if (e == Event::F2 || e == Event::Special({15})) {  // F2 or Ctrl+O
            show_context = !show_context;
            return true;
        }
        if (target_overlay_open) {
            std::lock_guard<std::mutex> lk(target_mutex);
            const int n = static_cast<int>(recent_targets.size());
            if (e == Event::ArrowDown) {
                target_overlay_selected = std::min(target_overlay_selected + 1, std::max(0, n - 1));
                return true;
            }
            if (e == Event::ArrowUp) {
                target_overlay_selected = std::max(target_overlay_selected - 1, 0);
                return true;
            }
            if (e == Event::Return && n > 0) {
                const auto selected = recent_targets[static_cast<size_t>(target_overlay_selected)];
                current_target = selected;
                if (selected.kind == TargetKind::SourceFile)
                    current_source = selected;
                else if (selected.kind == TargetKind::ArtifactFile)
                    current_artifact = selected;
                else if (selected.kind == TargetKind::TraceFile)
                    current_trace = selected;
                else if (selected.kind == TargetKind::PolicyFile)
                    current_policy = selected;
                save_workspace();
                {
                    std::lock_guard<std::mutex> history_lk(history_mutex);
                    history.push_back({Message::Role::System,
                        "Selected target: " + selected.path});
                }
                target_overlay_open = false;
                return true;
            }
            return true;
        }
        const bool input_focused = input_c->Focused();
        if (!input_focused && e == Event::Character('c')) {
            show_context = !show_context;
            return true;
        }

        // Log Viewer handled natively by FTXUI Component system
        return false;
    });

    screen.Loop(root);

    // Final save on clean exit
    if (!quit_requested && !session_path.empty()) {
        std::vector<Message> snapshot;
        {
            std::lock_guard<std::mutex> lk(history_mutex);
            snapshot = history;
        }
        save_session(session_path, snapshot);
    }
    return 0;
}

} // namespace t81::tui
