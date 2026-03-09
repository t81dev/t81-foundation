// SPDX-License-Identifier: MIT
// T81 TUI — snapshot / unit tests (no interactive terminal required)
//
// Covers:
//   1. Session persistence (save_session / load_session roundtrip)
//   2. Command palette content (all_commands returns expected entries)
//   3. FTXUI DOM rendering at fixed terminal size (golden snapshot)
//   4. Trit-probability bar arithmetic (fill widths)

#include "tooling/tui/common.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "test_runtime_check.hpp"

using namespace ftxui;
namespace fs = std::filesystem;

// ── 1. Session persistence ─────────────────────────────────────────────────

static void test_session_roundtrip() {
    const fs::path tmp = fs::temp_directory_path() / "t81_tui_snapshot_test.jsonl";
    fs::remove(tmp);

    const std::vector<t81::tui::Message> original = {
        {t81::tui::Message::Role::System, "Welcome."},
        {t81::tui::Message::Role::User,   "Hello, T81."},
        {t81::tui::Message::Role::Agent,  "Hello! Type /help for commands."},
        {t81::tui::Message::Role::User,   "Special chars: \"quotes\", \\backslash, \nnewline."},
    };

    T81_TEST_CHECK(t81::tui::save_session(tmp.string(), original));

    std::vector<t81::tui::Message> loaded;
    T81_TEST_CHECK(t81::tui::load_session(tmp.string(), loaded));
    T81_TEST_CHECK(loaded.size() == original.size());

    for (size_t i = 0; i < original.size(); ++i) {
        T81_TEST_CHECK(loaded[i].role == original[i].role);
        T81_TEST_CHECK(loaded[i].text == original[i].text);
    }

    fs::remove(tmp);
    std::cout << "  [PASS] session roundtrip\n";
}

// ── 2. Command palette content ─────────────────────────────────────────────

static void test_command_palette() {
    const auto cmds = t81::tui::all_commands();
    T81_TEST_CHECK(!cmds.empty());

    // Spot-check expected entries are present
    const std::vector<std::string> required_names = {
        "code build", "code run", "axion status", "trace show",
        "canonfs list", "weights verify",
    };
    for (const auto& req : required_names) {
        const bool found = std::any_of(cmds.begin(), cmds.end(),
            [&](const t81::tui::CommandEntry& e){ return e.name == req; });
        if (!found) {
            std::cerr << "  [FAIL] command palette missing entry: " << req << "\n";
            std::exit(1);
        }
    }

    // Every entry must have a non-empty cli_command
    for (const auto& e : cmds) {
        T81_TEST_CHECK(!e.name.empty());
        T81_TEST_CHECK(!e.cli_command.empty());
    }

    std::cout << "  [PASS] command palette (" << cmds.size() << " entries)\n";
}

// ── 3. FTXUI DOM snapshot ──────────────────────────────────────────────────

static void test_dom_snapshot() {
    // Build a representative status bar element (same structure as studio/agent)
    auto status = hbox({
        text(" [Tier 1]")   | color(Color::Cyan),
        text("  |  ")       | color(Color::GrayDark),
        text("Axion: Strict") | color(Color::Yellow),
        text("  |  ")       | color(Color::GrayDark),
        text("Trace: abc123…") | color(Color::Green),
        filler(),
    }) | bgcolor(Color::Black);

    // Render at 80×3
    auto screen = Screen::Create(Dimension::Fixed(80), Dimension::Fixed(3));
    Render(screen, status);
    const std::string rendered = screen.ToString();

    // Golden assertions: expected substrings must appear in the rendered output
    const std::vector<std::string> must_contain = {
        "[Tier 1]",
        "Axion: Strict",
        "Trace: abc123",
    };
    for (const auto& s : must_contain) {
        if (rendered.find(s) == std::string::npos) {
            std::cerr << "  [FAIL] DOM snapshot missing: \"" << s << "\"\n";
            std::cerr << "  Rendered output:\n" << rendered << "\n";
            std::exit(1);
        }
    }
    std::cout << "  [PASS] DOM snapshot\n";
}

// ── 4. Trit-probability bar fill widths ────────────────────────────────────

static void test_trit_bar_widths() {
    // Verify that the bar '#' fill count is proportional to probability.
    // We replicate the trit_bar math inline (static function not exported).
    constexpr int width = 16;
    auto fill_count = [&](float p) -> int {
        p = std::max(0.f, std::min(1.f, p));
        return static_cast<int>(p * width);
    };

    T81_TEST_CHECK(fill_count(0.0f) == 0);
    T81_TEST_CHECK(fill_count(1.0f) == width);
    T81_TEST_CHECK(fill_count(0.5f) == 8);
    T81_TEST_CHECK(fill_count(0.75f) == 12);

    // Probabilities summing to 1 must fit
    const float p_pos = 0.62f, p_zero = 0.22f, p_neg = 0.16f;
    const int f_pos  = fill_count(p_pos);
    const int f_zero = fill_count(p_zero);
    const int f_neg  = fill_count(p_neg);
    T81_TEST_CHECK(f_pos  >= 0 && f_pos  <= width);
    T81_TEST_CHECK(f_zero >= 0 && f_zero <= width);
    T81_TEST_CHECK(f_neg  >= 0 && f_neg  <= width);
    T81_TEST_CHECK(f_pos > f_zero);   // dominant trit fills more
    T81_TEST_CHECK(f_zero > f_neg);

    std::cout << "  [PASS] trit bar widths\n";
}

// ── 5. Session roundtrip with multiline and empty messages ─────────────────

static void test_session_edge_cases() {
    const fs::path tmp = fs::temp_directory_path() / "t81_tui_edge_test.jsonl";
    fs::remove(tmp);

    const std::vector<t81::tui::Message> original = {
        {t81::tui::Message::Role::User,   ""},                        // empty text
        {t81::tui::Message::Role::Agent,  "line1\nline2\nline3"},     // multiline
        {t81::tui::Message::Role::System, "tab\there and \"quotes\""}, // tab + quotes
    };

    T81_TEST_CHECK(t81::tui::save_session(tmp.string(), original));
    std::vector<t81::tui::Message> loaded;
    T81_TEST_CHECK(t81::tui::load_session(tmp.string(), loaded));
    T81_TEST_CHECK(loaded.size() == original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        T81_TEST_CHECK(loaded[i].role == original[i].role);
        T81_TEST_CHECK(loaded[i].text == original[i].text);
    }
    fs::remove(tmp);
    std::cout << "  [PASS] session edge cases\n";
}

// ── 6. Session parser adversarial cases ─────────────────────────────────────

static void test_session_parser_adversarial_cases() {
    const fs::path tmp = fs::temp_directory_path() / "t81_tui_json_parser_test.jsonl";
    fs::remove(tmp);

    {
        std::ofstream f(tmp);
        f << "{\"role\":\"user\",\"text\":\"abc\\\\\\\"def\"}\n";
        f << "{\"text\":\"reverse order\",\"role\":\"agent\"}\n";
    }

    std::vector<t81::tui::Message> loaded = {
        {t81::tui::Message::Role::System, "stale"},
    };
    T81_TEST_CHECK(t81::tui::load_session(tmp.string(), loaded));
    T81_TEST_CHECK(loaded.size() == 2);
    T81_TEST_CHECK(loaded[0].role == t81::tui::Message::Role::User);
    T81_TEST_CHECK(loaded[0].text == "abc\\\"def");
    T81_TEST_CHECK(loaded[1].role == t81::tui::Message::Role::Agent);
    T81_TEST_CHECK(loaded[1].text == "reverse order");

    std::ofstream(tmp, std::ios::trunc);
    loaded = {{t81::tui::Message::Role::User, "stale"}};
    T81_TEST_CHECK(t81::tui::load_session(tmp.string(), loaded));
    T81_TEST_CHECK(loaded.empty());

    fs::remove(tmp);
    std::cout << "  [PASS] session parser adversarial cases\n";
}

// ── 7. Palette filtering and windowing ──────────────────────────────────────

static void test_palette_filter_and_windowing() {
    const auto cmds = t81::tui::all_commands();
    T81_TEST_CHECK(cmds.size() >= 2);

    const auto axion = t81::tui::filter_palette(cmds, "AXION");
    T81_TEST_CHECK(axion.size() == 4);
    T81_TEST_CHECK(axion[0]->name == "axion status");
    T81_TEST_CHECK(axion[3]->name == "axion simulate");

    T81_TEST_CHECK(t81::tui::palette_window_start(0, 20, 10) == 0);
    T81_TEST_CHECK(t81::tui::palette_window_start(9, 20, 10) == 0);
    T81_TEST_CHECK(t81::tui::palette_window_start(10, 20, 10) == 1);
    T81_TEST_CHECK(t81::tui::palette_window_start(19, 20, 10) == 10);

    std::cout << "  [PASS] palette filter and windowing\n";
}

// ── 8. `split_lines` and scroll-indicator helpers ───────────────────────────

static void test_tui_helpers() {
    T81_TEST_CHECK(t81::tui::split_lines("").size() == 1);
    T81_TEST_CHECK(t81::tui::split_lines("")[0].empty());

    const auto lines = t81::tui::split_lines("a\nb\n");
    T81_TEST_CHECK(lines.size() == 2);
    T81_TEST_CHECK(lines[0] == "a");
    T81_TEST_CHECK(lines[1] == "b");

    T81_TEST_CHECK(
        t81::tui::scroll_indicator_text(10, 37, 10, " ↑↓") == " L11-20/37 ↑↓");
    T81_TEST_CHECK(
        t81::tui::scroll_indicator_text(0, 0, 22, " ↑↓") == " L0-0/0 ↑↓");

    std::cout << "  [PASS] tui helpers\n";
}

// ── main ───────────────────────────────────────────────────────────────────

int main() {
    std::cout << "T81 TUI snapshot tests\n";
    test_session_roundtrip();
    test_session_edge_cases();
    test_session_parser_adversarial_cases();
    test_command_palette();
    test_dom_snapshot();
    test_trit_bar_widths();
    test_palette_filter_and_windowing();
    test_tui_helpers();
    std::cout << "All TUI snapshot tests passed.\n";
    return 0;
}
