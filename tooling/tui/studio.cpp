// SPDX-License-Identifier: MIT
// T81 TUI — Human Operator Interface (t81 studio)
#include "tooling/tui/studio.hpp"
#include "tooling/tui/common.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

#if !defined(_WIN32)
#  include <unistd.h>
#endif

using namespace ftxui;

namespace t81::tui {

// Render a scrollable log block capped at `visible` rows.
static Element scrollable_log(const std::vector<std::string>& lines,
                               int scroll_offset, int visible = 22)
{
    Elements elems;
    const int n     = static_cast<int>(lines.size());
    const int start = std::max(0, std::min(scroll_offset, n - visible));
    const int end   = std::min(n, start + visible);
    for (int i = start; i < end; ++i)
        elems.push_back(text(lines[i]));
    if (elems.empty()) elems.push_back(text("(empty)") | color(Color::GrayDark));
    elems.push_back(hbox({
        filler(),
        text(scroll_indicator_text(start, n, visible, " ↑↓/PgUp/PgDn "))
            | color(Color::GrayDark),
    }));
    return vbox(std::move(elems));
}

// ── CanonFS browser helpers ────────────────────────────────────────────────

struct CanonEntry {
    std::string hash;
    std::string label;
    std::string raw;   // full line from `t81 canonfs list`
};

static std::vector<CanonEntry> fetch_canonfs() {
    const std::string raw = exec_argv({"t81", "canonfs", "list"});
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
    const std::string raw = exec_argv({"t81", "axion", "status"});
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
    const std::string raw = exec_argv({"t81", "trace", "diff", a, b});
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
    const std::string raw = exec_argv({"t81", "trace", "summary", path});
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

    // ── Navigation ──────────────────────────────────────────────────────────
    std::vector<std::string> nav_entries = {
        "Workspace", "Compiler", "Determinism",
        "CanonFS",   "Axion",    "Trace",  "REPL",
    };
    int nav_selected = 0;

    // ── Output log (scrollable) ──────────────────────────────────────────────
    std::vector<std::string> output_log = {
        "T81 Studio  —  Human Operator Interface",
        "Navigate with arrow keys, Enter to activate, Ctrl+P for palette.",
        "PgUp/PgDn or j/k scroll the log.  'q' or Escape to quit.",
        "",
    };
    // Seed with live system info (version + axion status)
    {
        const std::string ver = exec_argv({"t81", "version"});
        for (const auto& l : split_lines(ver)) output_log.push_back(l);
        output_log.push_back("");
        const std::string ax = exec_argv({"t81", "axion", "status"});
        for (const auto& l : split_lines(ax)) {
            output_log.push_back(l);
            // Also seed SessionState from the startup axion query
            if (l.find("Strict")     != std::string::npos) state.axion_mode = "Strict";
            else if (l.find("Audit") != std::string::npos) state.axion_mode = "Audit";
            else if (l.find("Permissive") != std::string::npos) state.axion_mode = "Permissive";
            else if (l.find("Disabled")   != std::string::npos) state.axion_mode = "Disabled";
        }
        output_log.push_back("");
    }
    std::mutex log_mutex;
    int log_scroll = 0;  // first visible line

    auto push_log = [&](const std::string& s) {
        std::lock_guard<std::mutex> lk(log_mutex);
        for (const auto& l : split_lines(s))
            output_log.push_back(l);
        // Auto-scroll to bottom (computed inside the same lock)
        log_scroll = std::max(0, static_cast<int>(output_log.size()) - 22);
    };

    // ── CanonFS browser state ────────────────────────────────────────────────
    std::vector<CanonEntry>  canon_entries;
    std::vector<std::string> canon_display;  // for Menu
    int                      canon_selected      = 0;
    int                      canon_detail_idx    = -1;   // last fetched detail index
    std::string              canon_detail_cache;          // cached detail output

    auto refresh_canon_detail = [&]() {
        const int idx = std::min(canon_selected,
                                 static_cast<int>(canon_entries.size()) - 1);
        canon_detail_idx = idx;
        if (idx >= 0 && !canon_entries[idx].hash.empty())
            canon_detail_cache = exec_argv(
                {"t81", "canonfs", "show", canon_entries[idx].hash});
        else
            canon_detail_cache.clear();
    };

    auto refresh_canonfs = [&]() {
        canon_entries = fetch_canonfs();
        canon_display.clear();
        for (const auto& e : canon_entries)
            canon_display.push_back(
                (e.hash.empty() ? "" : e.hash.substr(0, 12) + "…  ") + e.label);
        canon_selected = std::max(0, std::min(canon_selected,
            static_cast<int>(canon_entries.size()) - 1));
        canon_detail_idx   = -1;  // invalidate detail cache
        canon_detail_cache.clear();
        refresh_canon_detail();
    };
    refresh_canonfs();

    // ── Axion dashboard state ────────────────────────────────────────────────
    std::vector<AxionLine> axion_lines;
    int axion_scroll = 0;

    auto refresh_axion = [&]() {
        axion_lines  = fetch_axion_status();
        axion_scroll = 0;
    };
    refresh_axion();

    // ── Trace visualizer state ───────────────────────────────────────────────
    std::string              trace_file_a;
    std::string              trace_file_b;
    std::vector<TraceLine>   trace_lines;
    int                      trace_scroll = 0;
    bool                     trace_diff_mode = false;  // false=summary, true=diff

    auto refresh_trace = [&]() {
        trace_lines = trace_diff_mode
            ? fetch_trace_diff(trace_file_a, trace_file_b)
            : fetch_trace_summary(trace_file_a);
        trace_scroll = 0;
    };

    // ── Text inputs ──────────────────────────────────────────────────────────
    std::string repl_input;
    std::string compiler_input;  // Compiler view — separate from Determinism
    std::string determ_input;    // Determinism view — separate from Compiler
    std::string palette_query;
    bool        palette_open     = false;
    int         palette_selected = 0;
    int         palette_scroll   = 0;  // first visible row in the 10-row window

    // ── FTXUI components ─────────────────────────────────────────────────────
    auto screen = ScreenInteractive::Fullscreen();

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

    // ── Input event handlers ─────────────────────────────────────────────────

    repl_component = CatchEvent(repl_component, [&](Event e) -> bool {
        if (e != Event::Return || repl_input.empty()) return false;
        const std::string expr = repl_input;
        repl_input.clear();
        push_log("> " + expr);
        // Use mkstemp to avoid collision when multiple studio instances run.
        std::string tmp;
        char tmp_buf[] = "/tmp/t81_studio_repl_XXXXXX.t81";
        // mkstemps creates the file and returns an fd; suffix length = 4 (".t81")
#if defined(_WIN32)
        tmp = "/tmp/t81_studio_repl.t81";
        const bool ok = [&]() {
            FILE* f = fopen(tmp.c_str(), "w");
            if (!f) return false;
            fputs(expr.c_str(), f);
            fclose(f);
            return true;
        }();
#else
        const int fd = mkstemps(tmp_buf, 4);
        tmp = tmp_buf;
        const bool ok = (fd >= 0) && [&]() {
            FILE* f = fdopen(fd, "w");
            if (!f) { close(fd); return false; }
            fputs(expr.c_str(), f);
            fclose(f);
            return true;
        }();
#endif
        if (ok) {
            push_log(exec_argv({"t81", "code", "run", tmp}));
            std::remove(tmp.c_str());
        } else {
            push_log("[error: could not write temp file]");
        }
        return true;
    });

    compiler_comp = CatchEvent(compiler_comp, [&](Event e) -> bool {
        if (compiler_input.empty()) return false;
        const std::string path = compiler_input;
        if (e == Event::Return) {
            push_log("$ t81 code build " + path);
            push_log(exec_argv({"t81", "code", "build", path}));
            return true;
        }
        if (e == Event::Special("\x12")) {  // Ctrl+R → run
            push_log("$ t81 code run " + path);
            push_log(exec_argv({"t81", "code", "run", path}));
            return true;
        }
        if (e == Event::Special("\x0b")) {  // Ctrl+K → syntax check
            push_log("$ t81 code check " + path);
            push_log(exec_argv({"t81", "code", "check", path}));
            return true;
        }
        return false;
    });

    determ_comp = CatchEvent(determ_comp, [&](Event e) -> bool {
        if (e != Event::Return || determ_input.empty()) return false;
        const std::string path = determ_input;
        push_log("$ t81 determinism hash " + path);
        push_log(exec_argv({"t81", "determinism", "hash", path}));
        return true;
    });

    // Trace inputs: Enter on either field refreshes the visualizer
    trace_a_input = CatchEvent(trace_a_input, [&](Event e) -> bool {
        if (e != Event::Return) return false;
        refresh_trace();
        return true;
    });
    trace_b_input = CatchEvent(trace_b_input, [&](Event e) -> bool {
        if (e != Event::Return) return false;
        trace_diff_mode = !trace_file_b.empty();
        refresh_trace();
        return true;
    });

    auto layout = Container::Vertical({
        Container::Horizontal({
            nav_component, canon_menu,
            compiler_comp, determ_comp, repl_component,
            trace_a_input, trace_b_input,
        }),
        palette_input_c,
    });

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
            text("Ctrl+P: palette   r: refresh   q: quit ") | color(Color::GrayDark),
        }) | bgcolor(Color::Black);

        // Navigation sidebar
        auto sidebar = vbox({
            text(" Navigation") | bold | color(Color::Cyan),
            separator(),
            nav_component->Render(),
        }) | border | size(WIDTH, EQUAL, 22);

        const std::string& view = nav_entries[nav_selected];
        Element content_body;

        // ── Per-view content ─────────────────────────────────────────────────

        if (view == "REPL") {
            std::lock_guard<std::mutex> lk(log_mutex);
            content_body = vbox({
                scrollable_log(output_log, log_scroll) | flex,
                separator(),
                hbox({
                    text("-> ") | color(Color::Green),
                    repl_component->Render() | flex,
                }),
            });

        } else if (view == "Compiler") {
            std::lock_guard<std::mutex> lk(log_mutex);
            content_body = vbox({
                scrollable_log(output_log, log_scroll) | flex,
                separator(),
                hbox({
                    text("File: ") | color(Color::Cyan),
                    compiler_comp->Render() | flex,
                }),
                hbox({
                    text(" Enter") | color(Color::Green), text(": build  "),
                    text("Ctrl+R") | color(Color::Green), text(": run  "),
                    text("Ctrl+K") | color(Color::Green), text(": check"),
                }) | color(Color::GrayDark),
            });

        } else if (view == "Determinism") {
            std::lock_guard<std::mutex> lk(log_mutex);
            content_body = vbox({
                scrollable_log(output_log, log_scroll) | flex,
                separator(),
                hbox({
                    text("File: ") | color(Color::Cyan),
                    determ_comp->Render() | flex,
                }),
                text(" Enter to hash · Verify bit-exact determinism across runs")
                    | color(Color::GrayDark),
            });

        } else if (view == "CanonFS") {
            auto detail_lines = split_lines(canon_detail_cache.empty()
                ? "Select an artifact to inspect." : canon_detail_cache);
            Elements det_elems;
            for (const auto& l : detail_lines) det_elems.push_back(text(l));

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
            // Color-coded policy status, scrollable.
            const int vis = 22;
            const int n   = static_cast<int>(axion_lines.size());
            const int start = std::max(0, std::min(axion_scroll, n - vis));
            const int end   = std::min(n, start + vis);
            Elements elems;
            for (int i = start; i < end; ++i)
                elems.push_back(text(axion_lines[i].text) | color(axion_lines[i].color));
            elems.push_back(hbox({
                filler(),
                text(scroll_indicator_text(start, n, vis, " ↑↓/PgUp/PgDn  r: refresh "))
                    | color(Color::GrayDark),
            }));
            content_body = vbox({
                text(" Axion Policy Status  (r: refresh)") | bold | color(Color::Cyan),
                separator(),
                vbox(std::move(elems)) | flex,
            });

        } else if (view == "Trace") {
            // Two-pane: controls top, diff/summary below.
            const int vis   = 18;
            const int n     = static_cast<int>(trace_lines.size());
            const int start = std::max(0, std::min(trace_scroll, n - vis));
            const int end   = std::min(n, start + vis);
            Elements elems;
            for (int i = start; i < end; ++i) {
                Color c = trace_lines[i].diverged ? Color::Red : Color::White;
                elems.push_back(text(trace_lines[i].text) | color(c));
            }
            if (elems.empty())
                elems.push_back(text("(no trace data)") | color(Color::GrayDark));
            elems.push_back(hbox({
                filler(),
                text(scroll_indicator_text(start, n, vis, "  r: refresh "))
                    | color(Color::GrayDark),
            }));

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
                vbox(std::move(elems)) | flex,
            });

        } else {
            // Workspace: scrollable general output log
            std::lock_guard<std::mutex> lk(log_mutex);
            content_body = scrollable_log(output_log, log_scroll);
        }

        auto content_panel = vbox({
            text("  " + view) | bold | color(Color::White),
            separator(),
            content_body | flex,
        }) | border | flex;

        auto main_area = hbox({ sidebar, content_panel }) | flex;

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
        const std::string& view = nav_entries[nav_selected];

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

        // Ctrl+P — always active
        if (e == Event::Special("\x10")) {
            palette_open = !palette_open;
            palette_query.clear();
            palette_selected = 0;
            palette_scroll = 0;
            return true;
        }

        // Escape / q
        if (e == Event::Escape) {
            if (palette_open) { palette_open = false; return true; }
            screen.ExitLoopClosure()();
            return true;
        }
        if (!palette_open && !input_focused && e == Event::Character('q')) {
            screen.ExitLoopClosure()();
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
                const auto& sel = *filtered[palette_selected];
                push_log("$ " + sel.cli_command);
                push_log(exec_argv(split_command_words(sel.cli_command)));
                palette_open = false;
                nav_selected = 0;
                return true;
            }
            return false;
        }

        // ── 'r' refresh per-view ──────────────────────────────────────────
        if (!input_focused && e == Event::Character('r')) {
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

        // ── Scroll: PgDn / PgUp / j / k ──────────────────────────────────
        auto scroll = [&](int& offset, int delta, int max_lines) {
            offset = std::max(0, std::min(offset + delta, max_lines - 1));
        };

        if (view == "Workspace" || view == "Compiler" ||
            view == "Determinism" || view == "REPL") {
            std::lock_guard<std::mutex> lk(log_mutex);
            const int n = static_cast<int>(output_log.size());
            if (e == Event::PageDown || (!input_focused && e == Event::Character('J'))) {
                scroll(log_scroll, 10, n); return true;
            }
            if (e == Event::PageUp   || (!input_focused && e == Event::Character('K'))) {
                scroll(log_scroll, -10, n); return true;
            }
            if (!input_focused && e == Event::Character('j')) { scroll(log_scroll, 1, n);  return true; }
            if (!input_focused && e == Event::Character('k')) { scroll(log_scroll, -1, n); return true; }
        }
        if (view == "Axion") {
            const int n = static_cast<int>(axion_lines.size());
            if (e == Event::PageDown || (!input_focused && e == Event::Character('j'))) {
                scroll(axion_scroll, 5, n); return true;
            }
            if (e == Event::PageUp   || (!input_focused && e == Event::Character('k'))) {
                scroll(axion_scroll, -5, n); return true;
            }
        }
        if (view == "Trace") {
            const int n = static_cast<int>(trace_lines.size());
            if (e == Event::PageDown || (!input_focused && e == Event::Character('j'))) {
                scroll(trace_scroll, 5, n); return true;
            }
            if (e == Event::PageUp   || (!input_focused && e == Event::Character('k'))) {
                scroll(trace_scroll, -5, n); return true;
            }
        }

        return false;
    });

    screen.Loop(root);
    return 0;
}

} // namespace t81::tui
