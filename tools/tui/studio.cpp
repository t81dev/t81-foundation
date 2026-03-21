// SPDX-License-Identifier: MIT
// T81 TUI — Human Operator Interface (t81 studio)
#include "tools/tui/studio.hpp"
#include "tools/tui/common.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#if defined(_WIN32)
#  define NOMINMAX
#  include <windows.h>
#else
#  include <unistd.h>
#endif

using namespace ftxui;

namespace t81::tui {

namespace fs = std::filesystem;

static bool line_looks_like_error(const std::string& line) {
    std::string lower = line;
    for (char& c : lower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return lower.find("[error:") != std::string::npos ||
           lower.find("[exit status ") != std::string::npos ||
           lower.find("permission denied") != std::string::npos ||
           lower.find("not found") != std::string::npos;
}

static Element log_line(const std::string& line) {
    if (line_looks_like_error(line))
        return text(line) | color(Color::Red);
    if (line == "[no output]")
        return text(line) | color(Color::GrayDark);
    if (!line.empty() && (line[0] == '$' || line[0] == '>'))
        return text(line) | color(Color::Cyan);
    return text(line);
}

// ── CanonFS browser helpers ────────────────────────────────────────────────

struct CanonEntry {
    std::string hash;
    std::string label;
    std::string raw;   // full line from `t81 canonfs list`
};

struct FilePickerEntry {
    std::string name;
    std::string path;
    bool is_dir = false;
};

enum class BrowserSection {
    Files = 0,
    Artifacts,
    Traces,
    Recent,
};

static BrowserSection next_browser_section(BrowserSection section) {
    switch (section) {
    case BrowserSection::Files: return BrowserSection::Artifacts;
    case BrowserSection::Artifacts: return BrowserSection::Traces;
    case BrowserSection::Traces: return BrowserSection::Recent;
    case BrowserSection::Recent: return BrowserSection::Files;
    }
    return BrowserSection::Files;
}

static BrowserSection prev_browser_section(BrowserSection section) {
    switch (section) {
    case BrowserSection::Files: return BrowserSection::Recent;
    case BrowserSection::Artifacts: return BrowserSection::Files;
    case BrowserSection::Traces: return BrowserSection::Artifacts;
    case BrowserSection::Recent: return BrowserSection::Traces;
    }
    return BrowserSection::Files;
}

static std::vector<FilePickerEntry> list_picker_entries(
    const std::string& dir, const std::string& filter)
{
    std::vector<FilePickerEntry> out;
    std::error_code ec;
    fs::path base = dir.empty() ? fs::current_path(ec) : fs::path(dir);
    if (ec)
        return out;

    const std::string needle = [&]() {
        std::string lower = filter;
        for (char& c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return lower;
    }();

    if (base.has_parent_path()) {
        out.push_back({"..", base.parent_path().string(), true});
    }

    auto skip_recursive_dir = [](const fs::path& p) {
        const std::string name = p.filename().string();
        return name == ".git" || name == "build" || name == ".t81_canonfs" ||
               name == "third_party" || (!name.empty() && name[0] == '.');
    };

    std::vector<FilePickerEntry> dirs;
    std::vector<FilePickerEntry> files;
    const bool recursive = !needle.empty();
    size_t match_limit = recursive ? 250 : 0;
    if (recursive) {
        fs::recursive_directory_iterator it(base, ec), end;
        for (; !ec && it != end && match_limit > 0; ++it) {
            const auto& entry = *it;
            if (entry.is_directory(ec) && skip_recursive_dir(entry.path())) {
                it.disable_recursion_pending();
                continue;
            }
            if (entry.is_directory(ec))
                continue;
            const fs::path& p = entry.path();
            const std::string rel = fs::relative(p, base, ec).string();
            const std::string name = rel.empty() ? p.filename().string() : rel;
            std::string lower = name;
            for (char& c : lower)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower.find(needle) == std::string::npos)
                continue;
            files.push_back({name, p.string(), false});
            --match_limit;
        }
    } else {
        for (const auto& entry : fs::directory_iterator(base, ec)) {
            if (ec)
                break;
            const fs::path& p = entry.path();
            const std::string name = p.filename().string();
            FilePickerEntry item{name, p.string(), entry.is_directory(ec)};
            if (item.is_dir)
                dirs.push_back(std::move(item));
            else
                files.push_back(std::move(item));
        }
    }
    auto by_name = [](const FilePickerEntry& a, const FilePickerEntry& b) {
        return a.name < b.name;
    };
    std::sort(dirs.begin(), dirs.end(), by_name);
    std::sort(files.begin(), files.end(), by_name);
    out.insert(out.end(), dirs.begin(), dirs.end());
    out.insert(out.end(), files.begin(), files.end());
    return out;
}

static std::vector<FilePickerEntry> list_project_entries_by_extension(
    const std::string& root, const std::string& filter, const std::string& ext)
{
    std::vector<FilePickerEntry> out;
    std::error_code ec;
    const fs::path base = root.empty() ? fs::current_path(ec) : fs::path(root);
    if (ec || base.empty())
        return out;

    std::string needle = filter;
    for (char& c : needle)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    auto skip_recursive_dir = [](const fs::path& p) {
        const std::string name = p.filename().string();
        return name == ".git" || name == "build" || name == ".t81_canonfs" ||
               name == "third_party" || (!name.empty() && name[0] == '.');
    };

    size_t match_limit = 250;
    fs::recursive_directory_iterator it(base, ec), end;
    for (; !ec && it != end && match_limit > 0; ++it) {
        const auto& entry = *it;
        if (entry.is_directory(ec) && skip_recursive_dir(entry.path())) {
            it.disable_recursion_pending();
            continue;
        }
        if (entry.is_directory(ec))
            continue;
        const fs::path& p = entry.path();
        if (p.extension() != ext)
            continue;
        const std::string rel = fs::relative(p, base, ec).string();
        std::string lower = rel;
        for (char& c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (!needle.empty() && lower.find(needle) == std::string::npos)
            continue;
        out.push_back({rel.empty() ? p.filename().string() : rel, p.string(), false});
        --match_limit;
    }
    std::sort(out.begin(), out.end(), [](const FilePickerEntry& a, const FilePickerEntry& b) {
        return a.name < b.name;
    });
    return out;
}

static std::vector<CanonEntry> fetch_canonfs() {
    const std::string raw = exec_argv(t81_cli_argv({"canonfs", "ls"}));
    std::vector<CanonEntry> out;
    for (const auto& line : split_lines(raw)) {
        if (line.empty()) continue;
        CanonEntry e;
        e.raw = line;
        // Heuristic: first token is hash, rest is label
        auto sp = line.find(' ');
        if (sp != std::string::npos) {
            e.hash  = line.substr(0, sp);
            e.label = line.substr(sp + 1);
        } else {
            e.hash  = line;
            e.label = line;
        }
        out.push_back(e);
    }
    if (out.empty())
        out.push_back({"", "(no artifacts — run `t81 canonfs snapshot` first)", ""});
    return out;
}

// ── Axion dashboard helpers ────────────────────────────────────────────────

struct AxionLine { std::string text; Color color; };

static std::vector<AxionLine> fetch_axion_status() {
    const std::string raw = exec_argv(t81_cli_argv({"axion", "status"}));
    std::vector<AxionLine> out;
    for (const auto& line : split_lines(raw)) {
        Color c = Color::White;
        if (line.find("DENY")  != std::string::npos) c = Color::Red;
        else if (line.find("WARN")  != std::string::npos) c = Color::Yellow;
        else if (line.find("ALLOW") != std::string::npos) c = Color::Green;
        else if (line.find("error") != std::string::npos) c = Color::Red;
        out.push_back({line, c});
    }
    if (out.empty()) out.push_back({"(no Axion status — is an axion policy loaded?)", Color::GrayDark});
    return out;
}

// ── Trace visualizer helpers ───────────────────────────────────────────────

struct TraceLine { std::string text; bool diverged = false; };

static std::vector<TraceLine> fetch_trace_diff(const std::string& a,
                                                const std::string& b)
{
    if (a.empty() || b.empty())
        return {{"Enter two trace file paths above, then press Enter.", false}};
    const std::string raw = exec_argv(t81_cli_argv({"trace", "diff", a, b}));
    std::vector<TraceLine> out;
    for (const auto& line : split_lines(raw)) {
        bool div = (line.find("DIVERGE") != std::string::npos ||
                    line.find("differ")  != std::string::npos ||
                    (!line.empty() && line[0] == '-') ||
                    (!line.empty() && line[0] == '+'));
        out.push_back({line, div});
    }
    return out;
}

static std::vector<TraceLine> fetch_trace_summary(const std::string& path) {
    if (path.empty())
        return {{"Enter a trace file path above, then press Enter.", false}};
    const std::string raw = exec_argv(t81_cli_argv({"trace", "summary", path}));
    std::vector<TraceLine> out;
    for (const auto& line : split_lines(raw))
        out.push_back({line, false});
    return out;
}

// ── run_studio ─────────────────────────────────────────────────────────────

int run_studio(const std::vector<std::string>& /*args*/) {
    // ── Persistent state ────────────────────────────────────────────────────
    SessionState state;
    const std::vector<CommandEntry> palette_cmds = all_commands();
    std::error_code cwd_ec;
    std::string project_root = fs::current_path(cwd_ec).string();
    if (project_root.empty())
        project_root = ".";
    const std::string workspace_path = default_workspace_state_path();
    TargetRef current_target;
    TargetRef current_source;
    TargetRef current_artifact;
    TargetRef current_trace;
    TargetRef current_policy;
    TargetRef lhs_target;
    TargetRef rhs_target;
    std::vector<TargetRef> recent_targets;
    {
        WorkspaceState workspace;
        if (!workspace_path.empty() && load_workspace_state(workspace_path, workspace)) {
            if (!workspace.project_root.empty())
                project_root = workspace.project_root;
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

    // ── Navigation ──────────────────────────────────────────────────────────
    std::vector<std::string> nav_entries = {
        "Workspace", "Compiler", "Determinism",
        "CanonFS",   "Axion",    "Trace",  "REPL",
    };
    int nav_selected = 0;

    // ── Output log (scrollable) ──────────────────────────────────────────────
    std::vector<std::string> output_log = {
        "T81 Foundation Workspace  —  Human Operator Interface",
        "Navigate with arrow keys, Enter to activate, Ctrl+P for palette.",
        "PgUp/PgDn or j/k scroll the log.  'q' or Escape to quit.",
        "Bootstrapping runtime status…",
        "",
    };
    std::mutex log_mutex;
    int log_scroll = 0;  // first visible line
    std::mutex status_mutex;
    int pending_jobs = 0;
    std::string busy_status;
    std::string last_error;

    auto output_log_viewer = LogViewer([&]() {
        std::lock_guard<std::mutex> lk(log_mutex);
        Elements elems;
        for (const auto& line : output_log) {
            elems.push_back(log_line(line));
        }
        return elems;
    });

    auto push_log = [&](const std::string& s) {
        std::lock_guard<std::mutex> lk(log_mutex);
        for (const auto& l : split_lines(s))
            output_log.push_back(l);
        // Auto-scroll to bottom (computed inside the same lock)
        log_scroll = std::max(0, static_cast<int>(output_log.size()) - 22);
    };

    auto set_last_error = [&](const std::string& s) {
        if (!line_looks_like_error(s))
            return;
        std::lock_guard<std::mutex> lk(status_mutex);
        last_error = s.substr(0, 80);
    };

    // ── CanonFS browser state ────────────────────────────────────────────────
    std::vector<CanonEntry>  canon_entries;
    std::vector<std::string> canon_display;  // for Menu
    int                      canon_selected      = 0;
    int                      canon_detail_idx    = -1;   // last fetched detail index
    std::string              canon_detail_cache;          // cached detail output
    bool                     canon_detail_loading = false;
    std::mutex               canon_detail_mutex;

    // ── FTXUI components ─────────────────────────────────────────────────────
    auto screen = ScreenInteractive::Fullscreen();
    std::mutex task_mutex;
    std::vector<std::function<void()>> pending_ui_tasks;

    auto queue_ui = [&](std::function<void()> fn) {
        {
            std::lock_guard<std::mutex> lk(task_mutex);
            pending_ui_tasks.push_back(std::move(fn));
        }
        screen.PostEvent(Event::Custom);
    };

    auto begin_busy = [&](const std::string& label) {
        std::lock_guard<std::mutex> lk(status_mutex);
        ++pending_jobs;
        busy_status = label;
    };

    auto end_busy = [&]() {
        std::lock_guard<std::mutex> lk(status_mutex);
        pending_jobs = std::max(0, pending_jobs - 1);
        if (pending_jobs == 0)
            busy_status.clear();
    };

    auto run_async = [&](const std::string& label, std::function<void()> fn) {
        begin_busy(label);
        screen.PostEvent(Event::Custom);
        std::thread([&, fn = std::move(fn)]() mutable {
            fn();
            end_busy();
            screen.PostEvent(Event::Custom);
        }).detach();
    };

    auto refresh_canon_detail = [&]() {
        const int idx = std::min(canon_selected,
                                 static_cast<int>(canon_entries.size()) - 1);
        canon_detail_idx = idx;
        {
            std::lock_guard<std::mutex> lk(canon_detail_mutex);
            canon_detail_loading = true;
            canon_detail_cache = "Loading CanonFS artifact detail…";
        }
        if (idx < 0 || canon_entries[idx].hash.empty()) {
            std::lock_guard<std::mutex> lk(canon_detail_mutex);
            canon_detail_loading = false;
            canon_detail_cache.clear();
            return;
        }
        const std::string hash = canon_entries[idx].hash;
        run_async("Loading CanonFS detail", [&, idx, hash]() {
            const std::string detail = exec_argv(t81_cli_argv({"canonfs", "stat", hash}));
            set_last_error(detail);
            queue_ui([&, idx, detail]() {
                if (canon_detail_idx == idx) {
                    std::lock_guard<std::mutex> lk(canon_detail_mutex);
                    canon_detail_cache = detail;
                    canon_detail_loading = false;
                }
            });
        });
    };

    auto refresh_canonfs = [&]() {
        run_async("Refreshing CanonFS artifact list", [&]() {
            auto entries = fetch_canonfs();
            queue_ui([&, entries = std::move(entries)]() mutable {
                canon_entries = std::move(entries);
                canon_display.clear();
                for (const auto& e : canon_entries)
                    canon_display.push_back(
                        (e.hash.empty() ? "" : e.hash.substr(0, 12) + "…  ") + e.label);
                canon_selected = std::max(0, std::min(canon_selected,
                    static_cast<int>(canon_entries.size()) - 1));
                canon_detail_idx = -1;
                {
                    std::lock_guard<std::mutex> lk(canon_detail_mutex);
                    canon_detail_cache.clear();
                }
                refresh_canon_detail();
            });
        });
    };
    canon_entries = fetch_canonfs();
    for (const auto& e : canon_entries)
        canon_display.push_back(
            (e.hash.empty() ? "" : e.hash.substr(0, 12) + "…  ") + e.label);
    refresh_canon_detail();

    // ── Axion dashboard state ────────────────────────────────────────────────
    std::vector<AxionLine> axion_lines;
    int axion_scroll = 0;
    bool axion_loading = false;
    std::mutex axion_mutex;

    auto axion_log_viewer = LogViewer([&]() {
        std::lock_guard<std::mutex> lk(axion_mutex);
        Elements elems;
        for (const auto& line : axion_lines) {
            elems.push_back(text(line.text) | color(line.color));
        }
        if (axion_loading) {
            elems.push_back(text("Refreshing Axion status…") | color(Color::Yellow));
        }
        return elems;
    });

    auto refresh_axion = [&]() {
        {
            std::lock_guard<std::mutex> lk(axion_mutex);
            axion_loading = true;
        }
        run_async("Refreshing Axion status", [&]() {
            auto lines = fetch_axion_status();
            for (const auto& line : lines)
                set_last_error(line.text);
            queue_ui([&, lines = std::move(lines)]() mutable {
                std::lock_guard<std::mutex> lk(axion_mutex);
                axion_lines = std::move(lines);
                axion_scroll = 0;
                axion_loading = false;
            });
        });
    };
    axion_lines = fetch_axion_status();

    // ── Trace visualizer state ───────────────────────────────────────────────
    std::string              trace_file_a;
    std::string              trace_file_b;
    std::vector<TraceLine>   trace_lines;
    int                      trace_scroll = 0;
    bool                     trace_diff_mode = false;  // false=summary, true=diff
    bool                     trace_loading = false;
    std::mutex               trace_mutex;

    auto trace_log_viewer = LogViewer([&]() {
        std::lock_guard<std::mutex> lk(trace_mutex);
        Elements elems;
        for (const auto& line : trace_lines) {
            Color c = line.diverged ? Color::Red : Color::White;
            elems.push_back(text(line.text) | color(c));
        }
        if (elems.empty()) {
            elems.push_back(text("(no trace data)") | color(Color::GrayDark));
        }
        if (trace_loading) {
            elems.push_back(text("Refreshing trace output…") | color(Color::Yellow));
        }
        return elems;
    });

    auto refresh_trace = [&]() {
        {
            std::lock_guard<std::mutex> lk(trace_mutex);
            trace_loading = true;
        }
        const std::string a = trace_file_a;
        const std::string b = trace_file_b;
        const bool diff_mode = trace_diff_mode;
        run_async(diff_mode ? "Diffing trace files" : "Summarizing trace file", [&, a, b, diff_mode]() {
            auto lines = diff_mode ? fetch_trace_diff(a, b) : fetch_trace_summary(a);
            for (const auto& line : lines)
                set_last_error(line.text);
            queue_ui([&, lines = std::move(lines)]() mutable {
                std::lock_guard<std::mutex> lk(trace_mutex);
                trace_lines = std::move(lines);
                trace_scroll = 0;
                trace_loading = false;
            });
        });
    };

    // ── Text inputs ──────────────────────────────────────────────────────────
    std::string repl_input;
    std::string compiler_input;  // Compiler view — separate from Determinism
    std::string determ_input;    // Determinism view — separate from Compiler
    std::string palette_query;
    bool        palette_open     = false;
    int         palette_selected = 0;
    int         palette_scroll   = 0;  // first visible row in the 10-row window
    bool        picker_open      = false;
    std::string picker_dir       = project_root;
    std::string picker_filter;
    std::vector<FilePickerEntry> picker_entries = list_picker_entries(picker_dir, picker_filter);
    int         picker_selected  = 0;
    int         picker_scroll    = 0;
    int         picker_recent_selected = 0;
    bool        browser_focused  = false;
    BrowserSection browser_section = BrowserSection::Files;
    std::vector<FilePickerEntry> browser_artifacts;
    std::vector<FilePickerEntry> browser_traces;

    auto nav_component    = Menu(&nav_entries, &nav_selected);
    auto repl_component   = Input(&repl_input,     "Enter T81Lang expression…");
    auto compiler_comp    = Input(&compiler_input, "Enter .t81 file path…");
    auto determ_comp      = Input(&determ_input,   "Enter .t81 / artifact path…");
    auto trace_a_input    = Input(&trace_file_a,   "Trace file (summary or diff left)…");
    auto trace_b_input    = Input(&trace_file_b,   "Reference trace file (diff right)…");
    auto palette_input_c  = Input(&palette_query,  "Search commands…");

    MenuOption canon_menu_opts;
    canon_menu_opts.on_change = [&]() {
        if (canon_selected != canon_detail_idx)
            refresh_canon_detail();
    };
    auto canon_menu = Menu(&canon_display, &canon_selected, canon_menu_opts);

    run_async("Bootstrapping runtime status", [&]() {
        const std::string ver = exec_argv(t81_cli_argv({"version"}));
        const std::string ax = exec_argv(t81_cli_argv({"axion", "status"}));
        set_last_error(ver);
        set_last_error(ax);
        queue_ui([&, ver, ax]() {
            push_log(ver);
            push_log("");
            push_log(ax);
            push_log("");
            for (const auto& l : split_lines(ax)) {
                if (l.find("Strict") != std::string::npos) state.axion_mode = "Strict";
                else if (l.find("Audit") != std::string::npos) state.axion_mode = "Audit";
                else if (l.find("Permissive") != std::string::npos) state.axion_mode = "Permissive";
                else if (l.find("Disabled") != std::string::npos) state.axion_mode = "Disabled";
            }
        });
    });

    auto remember_target = [&](const TargetRef& target) {
        if (target.kind == TargetKind::None || target.path.empty())
            return;
        t81::tui::remember_target(recent_targets, current_target, current_source,
                                  current_artifact, current_trace, current_policy,
                                  target, 8);
        if (!workspace_path.empty()) {
            WorkspaceState workspace{
                project_root, current_target, current_source, current_artifact,
                current_trace, current_policy, lhs_target, rhs_target, recent_targets
            };
            (void)save_workspace_state(workspace_path, workspace);
        }
    };

    auto save_workspace_now = [&]() {
        if (workspace_path.empty())
            return;
        WorkspaceState workspace{
            project_root, current_target, current_source, current_artifact,
            current_trace, current_policy, lhs_target, rhs_target, recent_targets
        };
        (void)save_workspace_state(workspace_path, workspace);
    };

    auto set_target_from_path = [&](const std::string& path) {
        remember_target(make_target_from_path(path));
    };

    auto refresh_picker = [&]() {
        picker_entries = list_picker_entries(picker_dir, picker_filter);
        browser_artifacts = list_project_entries_by_extension(project_root, picker_filter, ".tisc");
        browser_traces = list_project_entries_by_extension(project_root, picker_filter, ".trace");
        picker_selected = std::max(0, std::min(
            picker_selected, static_cast<int>(picker_entries.size()) - 1));
        picker_scroll = palette_window_start(picker_selected,
                                             static_cast<int>(picker_entries.size()), 12);
    };

    auto open_picker = [&]() {
        picker_open = true;
        browser_focused = true;
        picker_filter.clear();
        browser_section = BrowserSection::Files;
        if (!current_target.path.empty()) {
            fs::path current_path(current_target.path);
            picker_dir = (current_target.kind == TargetKind::Directory)
                ? current_target.path
                : current_path.parent_path().string();
            if (picker_dir.empty())
                picker_dir = project_root;
        } else {
            picker_dir = project_root;
        }
        picker_selected = 0;
        picker_scroll = 0;
        picker_recent_selected = 0;
        refresh_picker();
    };
    open_picker();

    auto apply_picker_selection = [&](const std::string& path) {
        const std::string& view = nav_entries[nav_selected];
        set_target_from_path(path);
        if (view == "Compiler" || infer_target_kind(path) == TargetKind::SourceFile) {
            compiler_input = path;
            remember_target(make_target_from_path(path));
        } else if (view == "Determinism") {
            determ_input = path;
        } else if (view == "Trace" || infer_target_kind(path) == TargetKind::TraceFile) {
            if (trace_file_a.empty() || trace_a_input->Focused()) {
                trace_file_a = path;
            } else {
                trace_file_b = path;
                trace_diff_mode = true;
            }
            remember_target(make_target_from_path(path));
        }
        push_log("$ selected " + path);
    };

    // ── Input event handlers ─────────────────────────────────────────────────

    auto run_log_command = [&](const std::string& display,
                               std::vector<std::string> argv,
                               const std::string& busy_label) {
        push_log(display);
        run_async(busy_label, [&, argv = std::move(argv)]() {
            const std::string out = exec_argv(argv);
            set_last_error(out);
            push_log(out);
        });
    };

    auto build_current_target = [&]() -> bool {
        const std::string path = !current_source.path.empty() ? current_source.path : compiler_input;
        if (path.empty())
            return false;
        set_target_from_path(path);
        run_log_command("$ t81 code build " + path,
                        t81_cli_argv({"code", "build", path}),
                        "Building source file");
        return true;
    };

    auto run_current_target = [&]() -> bool {
        const std::string path = !current_artifact.path.empty()
            ? current_artifact.path
            : (!current_source.path.empty() ? current_source.path : compiler_input);
        if (path.empty())
            return false;
        set_target_from_path(path);
        run_log_command("$ t81 code run " + path,
                        t81_cli_argv({"code", "run", path}),
                        "Running source file");
        return true;
    };

    auto check_current_target = [&]() -> bool {
        const std::string path = !current_source.path.empty() ? current_source.path : compiler_input;
        if (path.empty())
            return false;
        set_target_from_path(path);
        run_log_command("$ t81 code check " + path,
                        t81_cli_argv({"code", "check", path}),
                        "Checking source file");
        return true;
    };

    auto write_repl_temp = [&](const std::string& expr, std::string& tmp) -> bool {
#if defined(_WIN32)
        char dir[MAX_PATH] = {0};
        if (GetTempPathA(MAX_PATH, dir) == 0)
            return false;
        char base[MAX_PATH] = {0};
        if (GetTempFileNameA(dir, "t81", 0, base) == 0)
            return false;
        std::string path = std::string(base) + ".t81";
        (void)DeleteFileA(base);
        HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                  FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;
        DWORD written = 0;
        const BOOL ok = WriteFile(file, expr.data(), static_cast<DWORD>(expr.size()),
                                  &written, nullptr);
        CloseHandle(file);
        if (!ok || written != expr.size()) {
            (void)DeleteFileA(path.c_str());
            return false;
        }
        tmp = std::move(path);
        return true;
#else
        char tmp_buf[] = "/tmp/t81_studio_repl_XXXXXX.t81";
        const int fd = mkstemps(tmp_buf, 4);
        if (fd < 0)
            return false;
        FILE* f = fdopen(fd, "w");
        if (!f) {
            close(fd);
            return false;
        }
        const bool ok = fputs(expr.c_str(), f) >= 0 && fclose(f) == 0;
        if (!ok) {
            std::remove(tmp_buf);
            return false;
        }
        tmp = tmp_buf;
        return true;
#endif
    };

    repl_component = CatchEvent(repl_component, [&](Event e) -> bool {
        if (e != Event::Return || repl_input.empty()) return false;
        const std::string expr = repl_input;
        repl_input.clear();
        push_log("> " + expr);
        std::string tmp;
        const bool ok = write_repl_temp(expr, tmp);
        if (ok) {
            run_async("Running REPL expression", [&, tmp]() {
                const std::string out = exec_argv(t81_cli_argv({"code", "run", tmp}));
                set_last_error(out);
                push_log(out);
                std::remove(tmp.c_str());
            });
        } else {
            push_log("[error: could not write temp file]");
            set_last_error("[error: could not write temp file]");
        }
        return true;
    });

    compiler_comp = CatchEvent(compiler_comp, [&](Event e) -> bool {
        const std::string path = !compiler_input.empty() ? compiler_input : current_source.path;
        if (path.empty()) return false;
        set_target_from_path(path);
        if (e == Event::Return) {
            run_log_command("$ t81 code build " + path,
                            t81_cli_argv({"code", "build", path}),
                            "Building source file");
            return true;
        }
        if (e == Event::Special("\x12")) {  // Ctrl+R → run
            run_log_command("$ t81 code run " + path,
                            t81_cli_argv({"code", "run", path}),
                            "Running source file");
            return true;
        }
        if (e == Event::Special("\x0b")) {  // Ctrl+K → syntax check
            run_log_command("$ t81 code check " + path,
                            t81_cli_argv({"code", "check", path}),
                            "Checking source file");
            return true;
        }
        return false;
    });

    determ_comp = CatchEvent(determ_comp, [&](Event e) -> bool {
        if (e != Event::Return) return false;
        const std::string path = !determ_input.empty() ? determ_input : current_target.path;
        if (path.empty()) return false;
        set_target_from_path(path);
        run_log_command("$ t81 determinism hash " + path,
                        t81_cli_argv({"determinism", "hash", path}),
                        "Hashing artifact determinism");
        return true;
    });

    // Trace inputs: Enter on either field refreshes the visualizer
    trace_a_input = CatchEvent(trace_a_input, [&](Event e) -> bool {
        if (e != Event::Return) return false;
        if (!trace_file_a.empty())
            set_target_from_path(trace_file_a);
        refresh_trace();
        return true;
    });
    trace_b_input = CatchEvent(trace_b_input, [&](Event e) -> bool {
        if (e != Event::Return) return false;
        trace_diff_mode = !trace_file_b.empty();
        if (!trace_file_b.empty())
            set_target_from_path(trace_file_b);
        refresh_trace();
        return true;
    });

    auto layout = Container::Vertical({
        Container::Horizontal({
            nav_component, canon_menu,
            compiler_comp, determ_comp, repl_component,
            trace_a_input, trace_b_input,
            output_log_viewer, axion_log_viewer, trace_log_viewer,
        }),
        palette_input_c,
    });

    // ── Renderer ─────────────────────────────────────────────────────────────
    auto renderer = Renderer(layout, [&]() -> Element {
        int busy_count = 0;
        std::string busy_text;
        std::string error_text;
        {
            std::lock_guard<std::mutex> lk(status_mutex);
            busy_count = pending_jobs;
            busy_text = busy_status;
            error_text = last_error;
        }
        // Status bar
        auto status = hbox({
            text(" 🛡️ VM [Tier " + std::to_string(state.vm_tier) + "]")
                | color(Color::Cyan),
            text("  |  ") | color(Color::GrayDark),
            text("✓ Axion: [" + state.axion_mode + "]")
                | color(Color::Yellow),
            text("  |  ") | color(Color::GrayDark),
            text("⇄ Active Trace: [" + state.trace_hash + "]")
                | color(Color::Green),
            busy_count > 0
                ? text("  |  Busy: " + busy_text + " ")
                    | color(Color::Yellow)
                : text(""),
            !error_text.empty()
                ? text("  |  Last error: " + error_text + " ")
                    | color(Color::Red)
                : text(""),
            filler(),
            text("Browser active: arrows browse, Tab sections, Enter opens, Backspace up, Esc nav ")
                | color(Color::GrayDark),
        }) | bgcolor(Color::Black);

        // Navigation sidebar
        auto sidebar = vbox({
            text(" Navigation") | bold | color(Color::Cyan),
            separator(),
            nav_component->Render(),
            separator(),
            text(" Workspace") | bold | color(Color::White),
            text("Root: " + compact_path(project_root, 18)) | color(Color::GrayDark),
            text("Current:") | color(Color::GrayDark),
            text(compact_path(current_target.path.empty()
                    ? "(none)"
                    : current_target.path, 18))
                | color(Color::Green),
            text(target_kind_label(current_target.kind)) | color(Color::GrayDark),
            text("LHS: " + compact_path(lhs_target.path.empty() ? "(none)" : lhs_target.path, 18))
                | color(Color::GrayDark),
            text("RHS: " + compact_path(rhs_target.path.empty() ? "(none)" : rhs_target.path, 18))
                | color(Color::GrayDark),
        }) | border | size(WIDTH, EQUAL, 22);

        const bool browser_recent_mode = browser_section == BrowserSection::Recent;
        std::vector<FilePickerEntry>* browser_entries = &picker_entries;
        std::string browser_label = "Files";
        if (browser_section == BrowserSection::Artifacts) {
            browser_entries = &browser_artifacts;
            browser_label = "Artifacts";
        } else if (browser_section == BrowserSection::Traces) {
            browser_entries = &browser_traces;
            browser_label = "Traces";
        } else if (browser_recent_mode) {
            browser_label = "Recent";
        }
        const int browser_n = static_cast<int>(browser_entries->size());
        const int browser_recent_n = static_cast<int>(recent_targets.size());
        if (!browser_recent_mode && browser_n > 0) {
            picker_selected = std::max(0, std::min(picker_selected, browser_n - 1));
            picker_scroll = palette_window_start(picker_selected, browser_n, 14);
        } else if (!browser_recent_mode) {
            picker_selected = 0;
            picker_scroll = 0;
        }
        if (browser_recent_mode && browser_recent_n > 0)
            picker_recent_selected =
                std::max(0, std::min(picker_recent_selected, browser_recent_n - 1));

        Elements browser_rows;
        const int browser_show = std::min(14, std::max(0, browser_n - picker_scroll));
        for (int i = 0; i < browser_show; ++i) {
            const int idx = picker_scroll + i;
            const auto& entry = (*browser_entries)[idx];
            const bool is_lhs = !lhs_target.path.empty() && lhs_target.path == entry.path;
            const bool is_rhs = !rhs_target.path.empty() && rhs_target.path == entry.path;
            auto row = hbox({
                text(entry.is_dir ? " [D] " : " [F] ")
                    | color(entry.is_dir ? Color::Cyan : Color::White),
                text(is_lhs ? "L" : (is_rhs ? "R" : " "))
                    | color(is_lhs ? Color::Yellow : (is_rhs ? Color::Green : Color::GrayDark)),
                text(compact_path(entry.is_dir ? entry.name + "/" : entry.name, 28)),
                filler(),
            });
            browser_rows.push_back((browser_focused && !browser_recent_mode && idx == picker_selected)
                ? row | inverted
                : row);
        }
        if (browser_rows.empty())
            browser_rows.push_back(text("  No matching files") | color(Color::GrayDark));

        Elements recent_rows;
        for (size_t i = 0; i < recent_targets.size(); ++i) {
            const bool is_lhs = !lhs_target.path.empty() && lhs_target.path == recent_targets[i].path;
            const bool is_rhs = !rhs_target.path.empty() && rhs_target.path == recent_targets[i].path;
            auto row = hbox({
                text(is_lhs ? " L " : (is_rhs ? " R " : "   "))
                    | color(is_lhs ? Color::Yellow : (is_rhs ? Color::Green : Color::GrayDark)),
                text("  " + compact_path(recent_targets[i].path, 30)),
                filler(),
            });
            recent_rows.push_back((browser_focused && browser_recent_mode &&
                static_cast<int>(i) == picker_recent_selected)
                ? row | inverted
                : row | color(Color::GrayDark));
        }
        if (recent_rows.empty())
            recent_rows.push_back(text("  (no recent targets)") | color(Color::GrayDark));

        auto browser_panel = vbox({
            text(" Browser") | bold | color(browser_focused ? Color::Cyan : Color::White),
            separator(),
            text("Dir: " + compact_path(picker_dir, 32)),
            text("Filter: " + (picker_filter.empty() ? "(none)" : picker_filter))
                | color(Color::GrayDark),
            separator(),
                hbox({
                    text(browser_section == BrowserSection::Files ? " Files " : " Files ")
                        | bold | color(browser_section == BrowserSection::Files ? Color::White : Color::GrayDark),
                    text(browser_section == BrowserSection::Artifacts ? " Artifacts " : " Artifacts ")
                        | bold | color(browser_section == BrowserSection::Artifacts ? Color::White : Color::GrayDark),
                    text(browser_section == BrowserSection::Traces ? " Traces " : " Traces ")
                        | bold | color(browser_section == BrowserSection::Traces ? Color::White : Color::GrayDark),
                    text(browser_recent_mode ? " Recent " : " Recent ")
                        | bold | color(browser_recent_mode ? Color::White : Color::GrayDark),
                }),
                text(browser_label + (browser_focused ? " [active]" : "")) | color(Color::GrayDark),
                vbox(std::move(browser_rows)),
                separator(),
                vbox(std::move(recent_rows)),
                separator(),
                text(browser_focused
                ? " Enter: open/use   Tab/[/]: sections   Backspace: up   =/Space: lhs/rhs   Esc: nav "
                : " F3/o: focus browser   Esc: return from inputs ")
                | color(Color::GrayDark),
        }) | border | color(browser_focused ? Color::Cyan : Color::White)
           | size(WIDTH, EQUAL, 36);

        const std::string& view = nav_entries[nav_selected];
        Element content_body;

        // ── Per-view content ─────────────────────────────────────────────────

        if (view == "REPL") {
            content_body = vbox({
                output_log_viewer->Render() | flex,
                separator(),
                hbox({
                    text("-> ") | color(Color::Green),
                    repl_component->Render() | flex,
                }),
            });

        } else if (view == "Compiler") {
            content_body = vbox({
                output_log_viewer->Render() | flex,
                separator(),
                hbox({
                    text("File: ") | color(Color::Cyan),
                    compiler_comp->Render() | flex,
                }),
                hbox({
                    text(" Enter") | color(Color::Green), text(": build  "),
                    text("Ctrl+R") | color(Color::Green), text(": run  "),
                    text("Ctrl+K") | color(Color::Green), text(": check  "),
                    text("F5/F6/F7") | color(Color::Green), text(": current target"),
                }) | color(Color::GrayDark),
            });

        } else if (view == "Determinism") {
            content_body = vbox({
                output_log_viewer->Render() | flex,
                separator(),
                hbox({
                    text("File: ") | color(Color::Cyan),
                    determ_comp->Render() | flex,
                }),
                text(" Enter to hash · Verify bit-exact determinism across runs")
                    | color(Color::GrayDark),
            });

        } else if (view == "CanonFS") {
            std::string detail_cache;
            bool detail_loading_now = false;
            {
                std::lock_guard<std::mutex> lk(canon_detail_mutex);
                detail_cache = canon_detail_cache;
                detail_loading_now = canon_detail_loading;
            }
            auto detail_lines = split_lines(detail_cache.empty()
                ? "Select an artifact to inspect." : detail_cache);
            Elements det_elems;
            for (const auto& l : detail_lines) det_elems.push_back(text(l));
            if (detail_loading_now)
                det_elems.push_back(text("Loading detail…") | color(Color::Yellow));

            content_body = hbox({
                vbox({
                    text(" Artifacts (r: refresh)") | bold | color(Color::Cyan),
                    separator(),
                    canon_menu->Render() | flex,
                }) | border | size(WIDTH, EQUAL, 40),
                vbox({
                    text(" Detail") | bold | color(Color::White),
                    separator(),
                    vbox(std::move(det_elems)) | flex,
                }) | border | flex,
            });

        } else if (view == "Axion") {
            content_body = vbox({
                text(" Axion Policy Status  (r: refresh)") | bold | color(Color::Cyan),
                separator(),
                axion_log_viewer->Render() | flex,
            });

        } else if (view == "Trace") {
            // Two-pane: controls top, diff/summary below.
            content_body = vbox({
                vbox({
                    hbox({
                        text("Trace A: ") | color(Color::Cyan),
                        trace_a_input->Render() | flex,
                    }),
                    hbox({
                        text("Trace B: ") | color(Color::GrayDark),
                        trace_b_input->Render() | flex,
                    }),
                    text(trace_diff_mode
                        ? " [diff mode] Enter on Trace B to diff  |  r: refresh"
                        : " [summary mode] Leave B empty for summary  |  r: refresh")
                        | color(Color::GrayDark),
                }) | border,
                separator(),
                trace_log_viewer->Render() | flex,
            });

        } else {
            // Workspace: scrollable general output log
            content_body = output_log_viewer->Render() | flex;
        }

        auto content_panel = vbox({
            text("  " + view) | bold | color(Color::White),
            separator(),
            hbox({
                text("Target: ") | color(Color::GrayDark),
                text(current_target.path.empty()
                    ? "(none selected)"
                    : compact_path(current_target.path, 64))
                    | color(Color::Green),
                filler(),
                !lhs_target.path.empty()
                    ? text("LHS ") | color(Color::Yellow)
                    : text(""),
                !rhs_target.path.empty()
                    ? text("RHS ") | color(Color::Yellow)
                    : text(""),
                filler(),
                !recent_targets.empty()
                    ? text("Recent: " + std::to_string(recent_targets.size()))
                        | color(Color::GrayDark)
                    : text(""),
            }),
            separator(),
            content_body | flex,
        }) | border | flex;

        auto top_bar = hbox({
            text(" T81 Foundation Workspace") | bold | color(Color::Cyan),
            filler(),
            text("[Cmd: Ctrl+P] ") | color(Color::GrayDark),
        });

        auto main_area = vbox({
            top_bar,
            hbox({ sidebar, browser_panel, content_panel }) | flex,
        }) | flex;

        // ── Command palette overlay ──────────────────────────────────────────
        if (palette_open) {
            auto filtered = filter_palette(palette_cmds, palette_query);
            const int n = static_cast<int>(filtered.size());
            if (n > 0) {
                palette_selected = std::max(0, std::min(palette_selected, n - 1));
                palette_scroll = palette_window_start(palette_selected, n, 10);
            } else {
                palette_selected = 0;
                palette_scroll = 0;
            }
            Elements items;
            const int show = std::min(10, std::max(0, n - palette_scroll));
            for (int i = 0; i < show; ++i) {
                const int idx = palette_scroll + i;
                auto row = hbox({
                    text("  " + filtered[idx]->name + "  ") | bold,
                    text(filtered[idx]->description) | color(Color::GrayDark),
                    filler(),
                });
                items.push_back(idx == palette_selected ? row | inverted : row);
            }
            if (items.empty())
                items.push_back(text("  No results") | color(Color::GrayDark));
            else if (n > 10)
                items.push_back(hbox({
                    filler(),
                    text(" " + std::to_string(palette_selected + 1) + "/" +
                         std::to_string(n) + " ")
                        | color(Color::GrayDark),
                }));

            auto palette_box = vbox({
                hbox({
                    text(" > ") | color(Color::Cyan),
                    palette_input_c->Render() | flex,
                }),
                separator(),
                vbox(std::move(items)),
                separator(),
                text(" Enter: run   Esc: close   ↑↓/PgUp/PgDn: move ")
                    | color(Color::GrayDark),
            }) | border
              | size(WIDTH, EQUAL, 66)
              | size(HEIGHT, LESS_THAN, 16);

            return dbox({
                vbox({ main_area, status }),
                vbox({
                    filler(),
                    hbox({ filler(), palette_box, filler() }),
                    filler(),
                }),
            });
        }

        return vbox({ main_area, status });
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

        const std::string& view = nav_entries[nav_selected];
        const bool canon_focused = canon_menu->Focused();
        (void)nav_component->Focused(); // Remove unused variable warning but maintain semantic completeness

        // True when any text-input field currently holds keyboard focus.
        // Single-character hotkeys must be suppressed in that case so that
        // typing 'q', 'r', 'j', 'k' in a path field is not misinterpreted.
        const bool input_focused =
            repl_component->Focused()  ||
            compiler_comp->Focused()   ||
            determ_comp->Focused()     ||
            trace_a_input->Focused()   ||
            trace_b_input->Focused()   ||
            palette_input_c->Focused();

        auto focus_current_view = [&]() {
            if (view == "Compiler") {
                compiler_comp->TakeFocus();
                return true;
            }
            if (view == "Determinism") {
                determ_comp->TakeFocus();
                return true;
            }
            if (view == "CanonFS") {
                canon_menu->TakeFocus();
                return true;
            }
            if (view == "Trace") {
                trace_a_input->TakeFocus();
                return true;
            }
            if (view == "REPL") {
                repl_component->TakeFocus();
                return true;
            }
            return false;
        };

        // Ctrl+P — always active
        if (e == Event::F3 || e == Event::Special({15}) ||
            (!input_focused && e == Event::Character('o'))) {
            if (!palette_open) {
                open_picker();
                return true;
            }
        }
        if (e == Event::Special("\x10")) {
            palette_open = !palette_open;
            palette_query.clear();
            palette_selected = 0;
            palette_scroll = 0;
            return true;
        }

        // Escape / q
        if (e == Event::Escape) {
            if (browser_focused) {
                browser_focused = false;
                picker_open = false;
                return true;
            }
            if (palette_open) { palette_open = false; return true; }
            if (input_focused || canon_focused) {
                nav_component->TakeFocus();
                return true;
            }
            screen.ExitLoopClosure()();
            return true;
        }
        if (!palette_open && !input_focused && e == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }

        if (!palette_open && !input_focused && !canon_focused && e == Event::Return) {
            return focus_current_view();
        }

        if (browser_focused) {
            std::vector<FilePickerEntry>* browser_entries = &picker_entries;
            if (browser_section == BrowserSection::Artifacts)
                browser_entries = &browser_artifacts;
            else if (browser_section == BrowserSection::Traces)
                browser_entries = &browser_traces;
            const int n = static_cast<int>(browser_entries->size());
            const int recent_n = static_cast<int>(recent_targets.size());
            auto picker_count = [&]() {
                return browser_section == BrowserSection::Recent ? recent_n : n;
            };
            if (e == Event::Character('[')) {
                browser_section = prev_browser_section(browser_section);
                return true;
            }
            if (e == Event::Character(']')) {
                browser_section = next_browser_section(browser_section);
                return true;
            }
            if (e == Event::ArrowDown) {
                if (browser_section == BrowserSection::Recent)
                    picker_recent_selected = std::min(picker_recent_selected + 1, std::max(0, recent_n - 1));
                else {
                    picker_selected = std::min(picker_selected + 1, std::max(0, n - 1));
                    picker_scroll = palette_window_start(picker_selected, n, 12);
                }
                return true;
            }
            if (e == Event::ArrowUp) {
                if (browser_section == BrowserSection::Recent)
                    picker_recent_selected = std::max(picker_recent_selected - 1, 0);
                else {
                    picker_selected = std::max(picker_selected - 1, 0);
                    picker_scroll = palette_window_start(picker_selected, n, 12);
                }
                return true;
            }
            if (e == Event::PageDown) {
                if (browser_section == BrowserSection::Recent)
                    picker_recent_selected = std::min(picker_recent_selected + 12, std::max(0, recent_n - 1));
                else {
                    picker_selected = std::min(picker_selected + 12, std::max(0, n - 1));
                    picker_scroll = palette_window_start(picker_selected, n, 12);
                }
                return true;
            }
            if (e == Event::PageUp) {
                if (browser_section == BrowserSection::Recent)
                    picker_recent_selected = std::max(picker_recent_selected - 12, 0);
                else {
                    picker_selected = std::max(picker_selected - 12, 0);
                    picker_scroll = palette_window_start(picker_selected, n, 12);
                }
                return true;
            }
            if (e == Event::Tab) {
                browser_section = next_browser_section(browser_section);
                return true;
            }
            if (e == Event::Backspace) {
                if (browser_section == BrowserSection::Recent) {
                    browser_section = BrowserSection::Files;
                } else if (!picker_filter.empty()) {
                    picker_filter.pop_back();
                } else if (browser_section == BrowserSection::Files) {
                    fs::path current(picker_dir);
                    if (current.has_parent_path())
                        picker_dir = current.parent_path().string();
                }
                refresh_picker();
                return true;
            }
            if (e == Event::Character('R') && browser_section == BrowserSection::Files) {
                project_root = picker_dir;
                refresh_picker();
                save_workspace_now();
                push_log("$ workspace root " + project_root);
                return true;
            }
            auto selected_browser_target = [&]() -> TargetRef {
                if (browser_section == BrowserSection::Recent)
                    return recent_targets[static_cast<size_t>(picker_recent_selected)];
                return make_target_from_path((*browser_entries)[picker_selected].path);
            };
            if ((e == Event::Character('=') || e == Event::Character(' ')) && picker_count() > 0) {
                const TargetRef selected = selected_browser_target();
                if (e == Event::Character('=')) {
                    lhs_target = selected;
                    push_log("$ lhs " + selected.path);
                } else {
                    rhs_target = selected;
                    push_log("$ rhs " + selected.path);
                }
                save_workspace_now();
                return true;
            }
            if (e == Event::Return && picker_count() > 0) {
                if (browser_section == BrowserSection::Recent) {
                    apply_picker_selection(recent_targets[picker_recent_selected].path);
                    browser_focused = false;
                    picker_open = false;
                } else {
                    const auto entry = (*browser_entries)[picker_selected];
                    if (entry.is_dir) {
                        picker_dir = entry.path;
                        picker_selected = 0;
                        refresh_picker();
                    } else {
                        apply_picker_selection(entry.path);
                        browser_focused = false;
                        picker_open = false;
                    }
                }
                return true;
            }
            if (browser_section == BrowserSection::Files && e.is_character()) {
                const std::string chars = e.character();
                if (!chars.empty() && chars[0] >= 32) {
                    picker_filter += chars;
                    refresh_picker();
                    return true;
                }
            }
            return true;
        }

        // ── Palette navigation ────────────────────────────────────────────
        if (palette_open) {
            auto filtered = filter_palette(palette_cmds, palette_query);
            const int n = static_cast<int>(filtered.size());
            if (e == Event::ArrowDown) {
                palette_selected = std::min(palette_selected + 1, std::max(0, n - 1));
                palette_scroll = palette_window_start(palette_selected, n, 10);
                return true;
            }
            if (e == Event::ArrowUp) {
                palette_selected = std::max(palette_selected - 1, 0);
                palette_scroll = palette_window_start(palette_selected, n, 10);
                return true;
            }
            if (e == Event::PageDown) {
                palette_selected = std::min(palette_selected + 10, std::max(0, n - 1));
                palette_scroll = palette_window_start(palette_selected, n, 10);
                return true;
            }
            if (e == Event::PageUp) {
                palette_selected = std::max(palette_selected - 10, 0);
                palette_scroll = palette_window_start(palette_selected, n, 10);
                return true;
            }
            if (e == Event::Return && !filtered.empty()) {
                // Clamp in case the query was narrowed after the last ArrowDown.
                palette_selected = std::min(palette_selected, n - 1);
                const auto sel = *filtered[palette_selected];
                push_log("$ " + sel.cli_command);
                run_async("Running palette command", [&, argv = split_command_words(sel.cli_command)]() {
                    std::vector<std::string> resolved = argv;
                    if (!resolved.empty() && resolved.front() == "t81")
                        resolved.front() = cli_program_path();
                    const std::string out = exec_argv(resolved);
                    set_last_error(out);
                    push_log(out);
                });
                palette_open = false;
                nav_selected = 0;
                return true;
            }
            return false;
        }

        // ── 'r' refresh per-view ──────────────────────────────────────────
        if (!palette_open && !browser_focused && !input_focused) {
            if (e == Event::F5)
                return build_current_target();
            if (e == Event::F6)
                return run_current_target();
            if (e == Event::F7)
                return check_current_target();
        }
        if (!input_focused && !browser_focused && e == Event::Character('r')) {
            if (view == "CanonFS") {
                refresh_canonfs();
                return true;
            }
            if (view == "Axion") {
                refresh_axion();
                return true;
            }
            if (view == "Trace") {
                trace_diff_mode = !trace_file_b.empty();
                refresh_trace();
                return true;
            }
            return false;
        }
        if (!input_focused && !browser_focused && e == Event::Character('d')) {
            if (view == "Trace" && !lhs_target.path.empty() && !rhs_target.path.empty()) {
                trace_file_a = lhs_target.path;
                trace_file_b = rhs_target.path;
                trace_diff_mode = true;
                refresh_trace();
                return true;
            }
            if (view == "Determinism" && !lhs_target.path.empty() && !rhs_target.path.empty()) {
                determ_input = lhs_target.path;
                run_log_command("$ t81 determinism diff " + lhs_target.path + " " + rhs_target.path,
                                t81_cli_argv({"determinism", "diff", lhs_target.path, rhs_target.path}),
                                "Diffing selected artifacts");
                return true;
            }
        }

        // ── Non-Log Component Interceptions ──────────────────────────────
        // Notice: Logs are now handled by LogViewer which hooks its own events.
        return false;
    });

    screen.Loop(root);
    return 0;
}

} // namespace t81::tui
