// tools/playground/src/wasm_api.cpp
//
// Emscripten-exported API for the T81Lang in-browser playground.
// Compiled with: emcc -s MODULARIZE=1 -s EXPORT_NAME='T81VM' --bind ...
//
// Exported function:
//   t81_run(source: string) -> string   (JSON: {"ok":bool,"output":str,"errors":str})

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif

#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "t81/cli/driver.hpp"
#include "t81/vm/vm.hpp"

namespace {

// Redirects a std::ostream to capture into a string during its lifetime.
class StreamCapture {
public:
    explicit StreamCapture(std::ostream& stream)
        : stream_(stream), old_buf_(stream.rdbuf(buf_.rdbuf())) {}

    ~StreamCapture() { stream_.rdbuf(old_buf_); }

    std::string str() const { return buf_.str(); }

private:
    std::ostream& stream_;
    std::streambuf* old_buf_;
    std::ostringstream buf_;
};

// JSON-escape a string (handles \n, \t, \", \\).
std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
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

std::string make_result(bool ok, const std::string& output, const std::string& errors) {
    return "{\"ok\":" + std::string(ok ? "true" : "false") +
           ",\"output\":\"" + json_escape(output) + "\"" +
           ",\"errors\":\"" + json_escape(errors) + "\"}";
}

std::string t81_run_impl(const std::string& source) {
    // Capture stderr (diagnostics from the compiler front-end).
    StreamCapture err_cap(std::cerr);

    // ── Compile ───────────────────────────────────────────────────────────
    auto maybe_program = t81::cli::build_program_from_source(source, "<playground>");
    std::string compile_errors = err_cap.str();

    if (!maybe_program) {
        return make_result(false, "", compile_errors.empty()
                                         ? "Compilation failed (unknown error)"
                                         : compile_errors);
    }

    // ── Run ───────────────────────────────────────────────────────────────
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(*maybe_program);

    // Capture any runtime stderr too.
    StreamCapture run_err_cap(std::cerr);

    constexpr std::size_t kMaxSteps = 500'000;
    auto result = vm->run_to_halt(kMaxSteps);
    std::string runtime_errors = run_err_cap.str();

    // Collect printed output.
    const auto& lines = vm->state().printed_output;
    std::string output;
    for (const auto& line : lines) {
        output += line;
        output += '\n';
    }

    if (!result) {
        // Trap occurred — still show partial output.
        std::string trap_msg = runtime_errors;
        if (trap_msg.empty()) {
            trap_msg = "Runtime error (trap)";
        }
        return make_result(false, output, trap_msg);
    }

    return make_result(true, output, compile_errors + runtime_errors);
}

}  // namespace

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(t81_playground) {
    emscripten::function("t81_run", &t81_run_impl);
}
#else
// Stub for native unit testing outside Emscripten.
extern "C" const char* t81_run(const char* source) {
    static std::string last;
    last = t81_run_impl(source ? source : "");
    return last.c_str();
}
#endif
