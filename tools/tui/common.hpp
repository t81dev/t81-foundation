// SPDX-License-Identifier: MIT
// T81 TUI — shared types and utilities
#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

namespace t81::tui {

// ── Command palette entry ──────────────────────────────────────────────────
struct CommandEntry {
    std::string name;         // Display name / search key
    std::string description;  // Short description shown in palette
    std::string cli_command;  // Underlying `t81 …` invocation
};

// Returns the full set of discoverable T81 commands for the palette.
std::vector<CommandEntry> all_commands();

// ── Session state ──────────────────────────────────────────────────────────
// Lightweight snapshot of runtime context displayed in status bars.
struct SessionState {
    std::string model_name  = "None";
    std::string axion_mode  = "Strict";
    int         vm_tier     = 1;
    std::string trace_hash  = "—";
    // Trit probability distribution derived from last llama-run inference.
    float       trit_pos    = 0.0f;  // P(+1)
    float       trit_zero   = 0.0f;  // P(0)
    float       trit_neg    = 0.0f;  // P(-1)
    int         infer_tokens = 0;    // Token count from last /infer run
};

enum class TargetKind {
    None,
    SourceFile,
    ArtifactFile,
    TraceFile,
    PolicyFile,
    Directory,
    CanonFsObject,
};

struct TargetRef {
    TargetKind   kind = TargetKind::None;
    std::string  path;
    std::string  label;
    std::string  canon_hash;
};

struct WorkspaceState {
    std::string project_root;
    TargetRef current_target;
    TargetRef current_source;
    TargetRef current_artifact;
    TargetRef current_trace;
    TargetRef current_policy;
    TargetRef lhs_target;
    TargetRef rhs_target;
    std::vector<TargetRef> recent_targets;
};

// ── Subprocess helpers ─────────────────────────────────────────────────────
// exec_command: runs a pre-built shell string via popen.
//   Use only for hardcoded commands that contain no user-supplied tokens.
std::string exec_command(const std::string& cmd);

// exec_argv: preferred API — spawns the process directly (no shell).
//   Each element of argv is passed as a separate argument, so shell
//   metacharacters in user-supplied paths or prompts are never interpreted.
//   stderr is merged with stdout in the returned string.
std::string exec_argv(const std::vector<std::string>& argv);
void set_cli_program_path(std::string path);
std::string cli_program_path();
std::vector<std::string> t81_cli_argv(std::initializer_list<std::string> tail);
std::vector<std::string> t81_cli_argv(const std::vector<std::string>& tail);

// Splits a shell-like command line into argv tokens without executing it.
// Supports simple single quotes, double quotes, and backslash escapes.
std::vector<std::string> split_command_words(const std::string& text);

// Small TUI helpers used by the renderers and snapshot tests.
std::string compact_path(const std::string& path, size_t width = 40);
TargetKind infer_target_kind(const std::string& path);
std::string target_kind_label(TargetKind kind);
TargetRef make_target_from_path(const std::string& path);
std::string format_target_summary(const TargetRef& target);
void remember_target(std::vector<TargetRef>& recent_targets, TargetRef& current_target,
                     TargetRef& current_source, TargetRef& current_artifact,
                     TargetRef& current_trace, TargetRef& current_policy,
                     const TargetRef& target, size_t max_recent = 8);
std::vector<std::string> split_lines(const std::string& s);
std::vector<const CommandEntry*> filter_palette(
    const std::vector<CommandEntry>& cmds, const std::string& query);
int palette_window_start(int selected, int count, int visible);
std::string scroll_indicator_text(
    int scroll_offset, int total_lines, int visible, const std::string& suffix);

// ── Agent conversation record ──────────────────────────────────────────────
struct Message {
    enum class Role { User, Agent, System };
    Role        role;
    std::string text;
};

// Serialises/deserialises a message list to/from JSONL for session persistence.
bool save_session(const std::string& path, const std::vector<Message>& msgs);
bool load_session(const std::string& path, std::vector<Message>& msgs);
std::string default_workspace_state_path();
bool save_workspace_state(const std::string& path, const WorkspaceState& state);
bool load_workspace_state(const std::string& path, WorkspaceState& state);

// ── LogViewer Component ────────────────────────────────────────────────────
class LogViewerBase : public ftxui::ComponentBase {
public:
    LogViewerBase(std::function<ftxui::Elements()> content_generator)
        : content_generator_(std::move(content_generator)) {}

    ftxui::Element Render() override {
        ftxui::Elements items = content_generator_();
        const int total_items = static_cast<int>(items.size());

        if (items.empty()) {
            items.push_back(ftxui::text("(empty)") | ftxui::color(ftxui::Color::GrayDark));
        } else {
            // Apply slicing for manual scrolling
            // We use box_.y_max - box_.y_min as an estimate of visible lines, or default to 20
            int visible = std::max(10, box_.y_max - box_.y_min);

            // Adjust scroll to be valid
            if (auto_scroll_ && total_items > visible) {
                scroll_idx_ = total_items - visible;
            } else {
                scroll_idx_ = std::max(0, std::min(scroll_idx_, total_items - visible));
            }

            int end_idx = std::min(total_items, scroll_idx_ + visible);
            ftxui::Elements sliced;
            for (int i = scroll_idx_; i < end_idx; ++i) {
                sliced.push_back(std::move(items[i]));
            }

            // Add scroll indicator text natively to the bottom of the list
            if (total_items > visible) {
                std::string ind = scroll_indicator_text(scroll_idx_, total_items, visible, " ↑↓/PgUp/PgDn ");
                sliced.push_back(ftxui::hbox({
                    ftxui::filler(),
                    ftxui::text(ind) | ftxui::color(ftxui::Color::GrayDark)
                }));
            }

            items = std::move(sliced);
        }

        auto inner = ftxui::vbox(std::move(items)) | ftxui::reflect(box_);

        // Return without yframe because we manually sliced to fit the box
        return inner | (Focused() ? ftxui::focus : ftxui::nothing);
    }

    bool OnEvent(ftxui::Event event) override {
        if (!Focused()) return false;

        int visible = std::max(10, box_.y_max - box_.y_min);

        if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k')) {
            scroll_idx_ -= 1;
            if (scroll_idx_ < 0) scroll_idx_ = 0;
            auto_scroll_ = false;
            return true;
        }
        if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j')) {
            scroll_idx_ += 1;
            auto_scroll_ = false;
            return true;
        }
        if (event == ftxui::Event::PageUp) {
            scroll_idx_ -= visible / 2;
            if (scroll_idx_ < 0) scroll_idx_ = 0;
            auto_scroll_ = false;
            return true;
        }
        if (event == ftxui::Event::PageDown) {
            scroll_idx_ += visible / 2;
            auto_scroll_ = false;
            return true;
        }
        if (event == ftxui::Event::Home) {
            scroll_idx_ = 0;
            auto_scroll_ = false;
            return true;
        }
        if (event == ftxui::Event::End) {
            auto_scroll_ = true;
            return true;
        }

        return false;
    }

    bool Focusable() const override { return true; }

private:
    std::function<ftxui::Elements()> content_generator_;
    int scroll_idx_ = 0;
    bool auto_scroll_ = true;
    ftxui::Box box_;
};

inline ftxui::Component LogViewer(std::function<ftxui::Elements()> content_generator) {
    return std::make_shared<LogViewerBase>(std::move(content_generator));
}

} // namespace t81::tui
