// SPDX-License-Identifier: MIT
// T81 TUI — AI-Native / Agentic Interface (t81 agent)
#include "tooling/tui/agent.hpp"
#include "tooling/tui/common.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

using namespace ftxui;
namespace fs = std::filesystem;

namespace t81::tui {

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

// ── Slash-command dispatch ─────────────────────────────────────────────────

static std::string handle_slash(
    const std::string& cmd_line,
    SessionState&      state,
    const std::string& session_path,
    const std::vector<Message>& history)
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
            "  /run <file>                      Compile and execute a .t81 file\n"
            "  /check <file>                    Syntax-check without compiling\n"
            "  /disasm <file>                   Disassemble TISC bytecode\n"
            "  /trace <file>                    Show execution trace\n"
            "  /hash <file>                     Compute determinism hash (updates context)\n"
            "  /infer <model> --policy <p> <q>  Run governed llama-run inference\n"
            "                                   (updates trit-probability display)\n"
            "  /axion                           Show Axion policy status (updates context)\n"
            "  /policy <action>                 Policy: list | validate <f> | status\n"
            "  /allow <hash>                    Append hash to active Axion policy whitelist\n"
            "  /write <file> <content>          Write content to file on disk\n"
            "  /tier <n>                        Set cognitive tier display (1-5)\n"
            "  /trits                           Toggle trit-probability display\n"
            "  /save                            Save session to disk\n"
            "  /clear                           Clear conversation history\n"
            "  /quit                            Exit\n"
            "\n"
            "Keyboard shortcuts:\n"
            "  PgUp / PgDn             Scroll conversation history\n"
            "  Escape                  Exit\n";
    }

    // ── Code lifecycle ───────────────────────────────────────────────────────
    if (verb == "compile" && !rest.empty()) {
        const std::string out = exec_argv(
            prepend_args({"t81", "code", "build"}, split_command_words(rest)));
        update_trace_state(state, out);
        // Also capture the artifact name for context
        auto h = extract_token(out, "hash");
        if (h.empty()) h = extract_token(out, "Hash");
        if (!h.empty()) state.trace_hash = h.substr(0, 12) + "…";
        return out;
    }
    if (verb == "run" && !rest.empty()) {
        const std::string out = exec_argv(
            prepend_args({"t81", "code", "run"}, split_command_words(rest)));
        update_trace_state(state, out);
        return out;
    }
    if (verb == "check" && !rest.empty()) {
        return exec_argv(prepend_args({"t81", "code", "check"}, split_command_words(rest)));
    }
    if (verb == "disasm" && !rest.empty()) {
        return exec_argv(prepend_args({"t81", "code", "disasm"}, split_command_words(rest)));
    }

    // ── Inference (/infer <model> [--policy <p>] <prompt>) ───────────────────
    if (verb == "infer") {
        if (rest.empty())
            return "[/infer requires: <model.gguf|sha3-256:hash> --policy <p> <prompt>]";
        const auto infer_args = split_command_words(rest);
        if (infer_args.empty())
            return "[/infer requires: <model.gguf|sha3-256:hash> --policy <p> <prompt>]";
        const std::string out =
            exec_argv(prepend_args({"t81", "internal", "llama-run"}, infer_args));
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
    if (verb == "trace" && !rest.empty()) {
        const std::string out = exec_argv(
            prepend_args({"t81", "trace", "show"}, split_command_words(rest)));
        update_trace_state(state, out);
        return out;
    }
    if (verb == "hash" && !rest.empty()) {
        const std::string out = exec_argv(
            prepend_args({"t81", "determinism", "hash"}, split_command_words(rest)));
        update_trace_state(state, out);
        return out;
    }

    // ── Axion / policy ───────────────────────────────────────────────────────
    if (verb == "axion") {
        const std::string out = exec_argv({"t81", "axion", "status"});
        update_axion_state(state, out);
        return out;
    }
    if (verb == "policy") {
        if (rest.empty()) rest = "list";
        const std::string out = exec_argv(
            prepend_args({"t81", "policy"}, split_command_words(rest)));
        // If it was a status query, update axion state too
        if (rest == "status") update_axion_state(state, out);
        return out;
    }
    if (verb == "allow" && !rest.empty()) {
        // Append the hash to the active policy's allowed-tensor-hashes list.
        // Delegates to the CLI which knows the policy file location.
        const std::string out = exec_argv(
            prepend_args({"t81", "policy", "allow-hash"}, split_command_words(rest)));
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
    std::vector<Message> history;
    std::mutex           history_mutex;
    bool                 show_trits     = false;
    bool                 quit_requested = false;
    int                  history_scroll = 0;   // first visible message index
    static const int     VISIBLE_MSGS   = 28;

    if (!resume_path.empty()) {
        load_session(resume_path, history);
        if (!history.empty())
            history_scroll = std::max(0,
                static_cast<int>(history.size()) - VISIBLE_MSGS);
    }

    // Bootstrap: sync context panel from live system state
    {
        const std::string ax = exec_argv({"t81", "axion", "status"});
        update_axion_state(state, ax);
    }

    // Welcome / resume message
    {
        std::lock_guard<std::mutex> lk(history_mutex);
        history.push_back({Message::Role::System,
            resume_path.empty()
                ? "T81 Agentic Interface. Type /help for commands."
                : "Session resumed from " + resume_path + ". Type /help for commands."});
        history_scroll = std::max(0,
            static_cast<int>(history.size()) - VISIBLE_MSGS);
    }

    std::string input_buf;

    // ── FTXUI setup ──────────────────────────────────────────────────────────
    auto screen  = ScreenInteractive::Fullscreen();
    auto input_c = Input(&input_buf, "Type a message or /command…");

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
            history_scroll = 0;
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
        std::string reply;
        if (!text_in.empty() && text_in.front() == '/') {
            reply = handle_slash(text_in, state, session_path, history);
        } else {
            reply = "[Agent] Received: \"" + text_in + "\"\n"
                    "Use slash commands to interact with T81 subsystems:\n"
                    "  /compile /run /check /disasm /trace /hash\n"
                    "  /axion /policy /allow /write /tier /trits /help";
        }

        {
            std::lock_guard<std::mutex> lk(history_mutex);
            history.push_back({Message::Role::Agent, reply});
            // Auto-scroll to bottom
            history_scroll = std::max(0,
                static_cast<int>(history.size()) - VISIBLE_MSGS);
        }

        // Auto-save after every turn
        if (!session_path.empty()) {
            std::vector<Message> snapshot;
            {
                std::lock_guard<std::mutex> lk(history_mutex);
                snapshot = history;
            }
            save_session(session_path, snapshot);
        }
        return true;
    });

    auto layout = Container::Vertical({ input_c });

    // ── Renderer ─────────────────────────────────────────────────────────────
    auto renderer = Renderer(layout, [&]() -> Element {
        // Status bar
        auto status = hbox({
            text(" [Tier " + std::to_string(state.vm_tier) + "]")
                | color(Color::Cyan),
            text("  |  ") | color(Color::GrayDark),
            text("Axion: " + state.axion_mode)
                | color(Color::Yellow),
            text("  |  ") | color(Color::GrayDark),
            text("Trace: " + state.trace_hash)
                | color(Color::Green),
            filler(),
            text("PgUp/PgDn: scroll   /help   Esc: exit ")
                | color(Color::GrayDark),
        }) | bgcolor(Color::Black);

        // Context panel
        auto context_panel = vbox({
            text(" Context") | bold | color(Color::Cyan),
            separator(),
            text(" Model:  " + state.model_name),
            text(" Tier:   " + std::to_string(state.vm_tier)),
            hbox({ text(" Axion:  "),
                   text(state.axion_mode)
                       | color(state.axion_mode == "Strict"  ? Color::Green  :
                               state.axion_mode == "Audit"   ? Color::Yellow :
                               state.axion_mode == "Disabled"? Color::Red    :
                                                               Color::White) }),
            hbox({ text(" Trace:  "),
                   text(state.trace_hash) | color(Color::Green) }),
            separator(),
            text(" Trit Probs:") | color(Color::GrayDark),
            show_trits
                ? (state.infer_tokens > 0
                    ? vbox({
                        trit_bar(state.trit_pos, state.trit_zero, state.trit_neg),
                        text(" n=" + std::to_string(state.infer_tokens) + " tokens")
                            | color(Color::GrayDark),
                      })
                    : vbox({
                        trit_bar(state.trit_pos, state.trit_zero, state.trit_neg),
                        text(" run /infer to populate") | color(Color::GrayDark),
                      }))
                : (text(" [/trits to enable]") | color(Color::GrayDark)),
            filler(),
            separator(),
            text(" Session:") | color(Color::GrayDark),
            paragraph(session_path.empty() ? "(none)" : compact_session_path(session_path))
                | color(Color::GrayDark),
        }) | border | size(WIDTH, LESS_THAN, 30);

        // Conversation history (scrollable)
        Elements msgs;
        std::vector<Message> history_snapshot;
        int history_start = 0;
        int history_end = 0;
        int history_size = 0;
        {
            std::lock_guard<std::mutex> lk(history_mutex);
            history_size = static_cast<int>(history.size());
            history_start = std::max(0, std::min(history_scroll, history_size - 1));
            history_end = std::min(history_size, history_start + VISIBLE_MSGS);
            history_snapshot.assign(history.begin() + history_start, history.begin() + history_end);
        }
        for (const auto& m : history_snapshot) {
            switch (m.role) {
            case Message::Role::User:
                msgs.push_back(hbox({
                    text("[You]   ") | bold | color(Color::Green),
                    paragraph(m.text),
                }));
                break;
            case Message::Role::Agent:
                msgs.push_back(hbox({
                    text("[Agent] ") | bold | color(Color::Cyan),
                    paragraph(m.text),
                }));
                break;
            case Message::Role::System:
                msgs.push_back(
                    text("* " + m.text) | color(Color::GrayDark) | dim
                );
                break;
            }
            msgs.push_back(text(""));
        }
        if (history_size > VISIBLE_MSGS) {
            msgs.push_back(hbox({
                filler(),
                text(" msg " + std::to_string(history_start + 1) + "-" +
                     std::to_string(history_end) + "/" + std::to_string(history_size) +
                     "  PgUp/PgDn ")
                    | color(Color::GrayDark),
            }));
        }

        auto history_pane = vbox({
            text(" Interaction History") | bold | color(Color::White),
            separator(),
            vbox(std::move(msgs)) | flex,
            separator(),
            hbox({
                text(" >> ") | color(Color::Green),
                input_c->Render() | flex,
            }),
        }) | border | flex;

        return vbox({
            hbox({ history_pane, context_panel }) | flex,
            status,
        });
    });

    // ── Global key handler ───────────────────────────────────────────────────
    auto root = CatchEvent(renderer, [&](Event e) -> bool {
        if (e == Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }
        // History scroll
        {
            std::lock_guard<std::mutex> lk(history_mutex);
            const int n = static_cast<int>(history.size());
            if (e == Event::PageUp) {
                history_scroll = std::max(0, history_scroll - (VISIBLE_MSGS / 2));
                return true;
            }
            if (e == Event::PageDown) {
                history_scroll = std::min(std::max(0, n - VISIBLE_MSGS),
                                          history_scroll + (VISIBLE_MSGS / 2));
                return true;
            }
        }
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
