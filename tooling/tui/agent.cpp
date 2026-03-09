// SPDX-License-Identifier: MIT
// RFC-0033: Dual TUI Frontends — AI-Native / Agentic Interface (t81 agent)
//
// Phase 1 scope: conversation pane, context panel, slash commands,
// trit-probability readout placeholder, session save/resume.
#include "tooling/tui/agent.hpp"
#include "tooling/tui/common.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

using namespace ftxui;

namespace t81::tui {

// ── Slash-command dispatch ─────────────────────────────────────────────────
// Returns the agent's reply text, or empty string if unknown.
static std::string handle_slash(
    const std::string& cmd_line,
    SessionState&      state,
    const std::string& session_path,
    const std::vector<Message>& history)
{
    // Tokenise
    std::istringstream ss(cmd_line.substr(1));  // strip leading '/'
    std::string verb;
    ss >> verb;
    std::string rest;
    std::getline(ss, rest);
    if (!rest.empty() && rest.front() == ' ') rest.erase(0, 1);

    if (verb == "help") {
        return
            "Available slash commands:\n"
            "  /compile <file>    Compile a .t81 source file\n"
            "  /run <file>        Compile and execute a .t81 file\n"
            "  /trace <file>      Show execution trace for a bytecode file\n"
            "  /policy <action>   Axion policy commands (list, validate, status)\n"
            "  /hash <file>       Compute determinism hash for a file\n"
            "  /axion             Show current Axion policy status\n"
            "  /tier <n>          Set displayed cognitive tier (1-5)\n"
            "  /trits             Toggle trit-probability display\n"
            "  /save              Save session to disk\n"
            "  /clear             Clear conversation history\n"
            "  /quit              Exit the agent interface\n";
    }
    if (verb == "compile" && !rest.empty()) {
        return exec_command("t81 code build " + rest + " 2>&1");
    }
    if (verb == "run" && !rest.empty()) {
        return exec_command("t81 code run " + rest + " 2>&1");
    }
    if (verb == "trace" && !rest.empty()) {
        return exec_command("t81 trace show " + rest + " 2>&1");
    }
    if (verb == "hash" && !rest.empty()) {
        return exec_command("t81 determinism hash " + rest + " 2>&1");
    }
    if (verb == "axion") {
        std::string out = exec_command("t81 axion status 2>&1");
        // Update state.axion_mode from first line if possible
        return out;
    }
    if (verb == "policy") {
        if (rest.empty()) rest = "list";
        return exec_command("t81 policy " + rest + " 2>&1");
    }
    if (verb == "tier" && !rest.empty()) {
        try { state.vm_tier = std::stoi(rest); } catch (...) {}
        return "Cognitive tier set to " + std::to_string(state.vm_tier) + ".";
    }
    if (verb == "save") {
        if (session_path.empty())
            return "No session path set. Launch with --session <path> to enable auto-save.";
        return save_session(session_path, history)
            ? "Session saved to " + session_path + "."
            : "Failed to save session.";
    }
    return "[unknown command /" + verb + " — try /help]";
}

// ── Trit probability bar ───────────────────────────────────────────────────
// Renders a simple ASCII bar for P(+1), P(0), P(-1).
// In Phase 1 these are placeholder values; Phase 3 wires real inference data.
static Element trit_bar(float p_pos, float p_zero, float p_neg) {
    // Clamp
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
    // ── Parse flags ─────────────────────────────────────────────────────────
    std::string resume_path;
    std::string session_path;
    for (size_t i = 0; i < args.size(); ++i) {
        if ((args[i] == "--resume" || args[i] == "--session") && i + 1 < args.size()) {
            if (args[i] == "--resume")  resume_path  = args[i + 1];
            if (args[i] == "--session") session_path = args[i + 1];
            ++i;
        }
    }

    // ── State ────────────────────────────────────────────────────────────────
    SessionState state;
    std::vector<Message> history;
    std::mutex history_mutex;
    bool show_trits = false;
    bool quit_requested = false;

    if (!resume_path.empty()) {
        load_session(resume_path, history);
        if (session_path.empty()) session_path = resume_path;
    }

    // Welcome message
    {
        std::lock_guard<std::mutex> lk(history_mutex);
        history.push_back({Message::Role::System,
            "T81 Agentic Interface (RFC-0033 Phase 1). Type /help for commands."});
    }

    // Input buffer
    std::string input_buf;

    // ── FTXUI setup ──────────────────────────────────────────────────────────
    auto screen = ScreenInteractive::Fullscreen();
    auto input_c = Input(&input_buf, "Type a message or /command…");

    // Submit on Enter
    input_c = CatchEvent(input_c, [&](Event e) -> bool {
        if (e != Event::Return || input_buf.empty()) return false;
        const std::string text_in = input_buf;
        input_buf.clear();

        // Check for /quit early so we don't push a message
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
                std::string("Trit probability display ") + (show_trits ? "ON." : "OFF.")});
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
            // Phase 1 placeholder: echo back with a helpful hint
            reply = "[Agent] Received: \"" + text_in + "\"\n"
                    "Tip: Use slash commands (/compile, /run, /trace, /axion, /help) "
                    "to interact with T81 subsystems.\n"
                    "Full LLM-backend integration arrives in RFC-0033 Phase 3.";
        }

        {
            std::lock_guard<std::mutex> lk(history_mutex);
            history.push_back({Message::Role::Agent, reply});
        }

        // Auto-save
        if (!session_path.empty()) {
            std::lock_guard<std::mutex> lk(history_mutex);
            save_session(session_path, history);
        }
        return true;
    });

    auto layout = Container::Vertical({ input_c });

    // ── Renderer ─────────────────────────────────────────────────────────────
    auto renderer = Renderer(layout, [&]() -> Element {
        // Status bar
        auto status = hbox({
            text(" [VM Tier " + std::to_string(state.vm_tier) + "]")
                | color(Color::Cyan),
            text("  |  ") | color(Color::GrayDark),
            text("Axion: " + state.axion_mode)
                | color(Color::Yellow),
            text("  |  ") | color(Color::GrayDark),
            text("Trace: " + state.trace_hash)
                | color(Color::Green),
            filler(),
            text("/quit: exit   /help: commands ") | color(Color::GrayDark),
        }) | bgcolor(Color::Black);

        // Context panel (right column)
        auto context_panel = vbox({
            text(" Context") | bold | color(Color::Cyan),
            separator(),
            text(" Model:  " + state.model_name),
            text(" Tier:   " + std::to_string(state.vm_tier)),
            text(" Axion:  " + state.axion_mode),
            text(" Trace:  " + state.trace_hash),
            separator(),
            text(" Trit Probs:") | color(Color::GrayDark),
            show_trits
                ? trit_bar(0.62f, 0.22f, 0.16f)
                : (text(" [/trits to enable]") | color(Color::GrayDark)),
            filler(),
            separator(),
            text(" Session:") | color(Color::GrayDark),
            text(session_path.empty() ? " (none)" : " " + session_path)
                | color(Color::GrayDark),
        }) | border | size(WIDTH, EQUAL, 28);

        // Conversation history
        Elements msgs;
        {
            std::lock_guard<std::mutex> lk(history_mutex);
            const size_t start =
                history.size() > 30 ? history.size() - 30 : 0;
            for (size_t i = start; i < history.size(); ++i) {
                const auto& m = history[i];
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

        auto main_area = hbox({ history_pane, context_panel }) | flex;

        return vbox({ main_area, status });
    });

    auto root = CatchEvent(renderer, [&](Event e) -> bool {
        if (e == Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(root);

    // Final auto-save on clean exit
    if (!session_path.empty() && !quit_requested) {
        std::lock_guard<std::mutex> lk(history_mutex);
        save_session(session_path, history);
    }
    return 0;
}

} // namespace t81::tui
