#include "t81/cli/driver.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

namespace {

struct CerrRedirect {
    CerrRedirect() : old_buf(std::cerr.rdbuf(buffer.rdbuf())) {}
    ~CerrRedirect() { std::cerr.rdbuf(old_buf); }
    std::string str() const { return buffer.str(); }

    std::ostringstream buffer;
    std::streambuf* old_buf = nullptr;
};

std::string capture_diagnostics(const std::string& source,
                                const std::string& diag_label = "<diagnostic>") {
    CerrRedirect redirect;
    auto program = t81::cli::build_program_from_source(source, diag_label);
    assert(!program);
    return redirect.str();
}

void assert_contains(const std::string& output, const std::string& pattern, const char* label) {
    if (output.find(pattern) == std::string::npos) {
        std::cerr << "[" << label << "] diagnostic output missing '" << pattern << "'\n";
        std::cerr << output << '\n';
        assert(false);
    }
}

} // namespace

int main() {
    const std::string option_source = R"(
fn main() -> i32 {
    let maybe: Option[i32] = Some(true);
    return match (maybe) {
        Some(v) => v;
        None => 0;
    };
    return 0;
}
)";

    const std::string loop_source = R"(
fn main() -> i32 {
    loop {
        break;
    }
    return 0;
}
)";

    const std::string generic_source = R"(
fn main() -> i32 {
    let missing: Option[] = Some(1);
    return 0;
}
)";

    const std::string match_source = R"(
fn main() -> i32 {
    let maybe: Option[i32] = Some(1);
    return match (maybe) {
        Some(v) => v;
    };
}
)";

    {
        auto output = capture_diagnostics(option_source, "option");
        assert_contains(output, "Some(true);", "option");
        assert_contains(output, "Option payload", "option");
        assert_contains(output, "error:", "option");
        assert_contains(output, "^", "option");
    }

    {
        auto output = capture_diagnostics(loop_source, "loop");
        assert_contains(output, "loop {", "loop");
        assert_contains(output, "Loops must be annotated with '@bounded(...)'.", "loop");
        assert_contains(output, "^", "loop");
    }

    {
        auto output = capture_diagnostics(generic_source, "generic");
        assert_contains(output, "Option[]", "generic");
        assert_contains(output, "Generic type requires at least one parameter.", "generic");
        assert_contains(output, "^", "generic");
    }

    {
        auto output = capture_diagnostics(match_source, "match");
        assert_contains(output, "Some(v) => v;", "match");
        assert_contains(output, "requires 'None' arm", "match");
        assert_contains(output, "^", "match");
    }

    return 0;
}
