#include "t81/cli/driver.hpp"
#include "t81/cli/logging.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/tisc/binary_io.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace t81::vm {
std::string to_string(Trap trap) {
    switch (trap) {
        case Trap::None: return "None";
        case Trap::InvalidMemory: return "InvalidMemory";
        case Trap::IllegalInstruction: return "IllegalInstruction";
        case Trap::DivideByZero: return "DivideByZero";
        case Trap::BoundsFault: return "BoundsFault";
        case Trap::SecurityFault: return "SecurityFault";
        case Trap::TrapInstruction: return "TrapInstruction";
    }
    return "UnknownTrap";
}
}

int trap_exit_code(t81::vm::Trap trap) {
    using T = t81::vm::Trap;
    switch (trap) {
        case T::None:               return 0;
        case T::DivideByZero:       return 10;
        case T::InvalidMemory:      return 11;
        case T::BoundsFault:        return 12;
        case T::SecurityFault:      return 13;
        case T::IllegalInstruction: return 14;
        case T::TrapInstruction:    return 15;
        default:                    return 1;
    }
}

namespace {
std::vector<std::string> split_lines(std::string_view content);

void print_semantic_diagnostics(const t81::frontend::SemanticAnalyzer& analyzer,
                                std::string_view primary_source,
                                const std::string* source) {
    std::vector<std::string> lines;
    if (source) {
        lines = split_lines(*source);
    }
    for (const auto& diag : analyzer.diagnostics()) {
        const std::string file = diag.file.empty() ? "<source>" : diag.file;
        std::cerr << file << ':' << diag.line << ':' << diag.column
                  << ": error: " << diag.message << '\n';

        bool print_context = source &&
                             (file == primary_source || file == "<source>" || primary_source.empty());
        if (print_context) {
            if (diag.line > 0 && diag.line <= static_cast<int>(lines.size())) {
                const std::string& context = lines[diag.line - 1];
                std::cerr << "    " << context << '\n';
                int indent = std::max(0, diag.column - 1);
                if (indent > static_cast<int>(context.size())) {
                    indent = static_cast<int>(context.size());
                }
                std::cerr << "    " << std::string(indent, ' ') << "^\n";
            }
        }
    }
}

std::string structural_kind_name(t81::tisc::StructuralKind kind) {
    switch (kind) {
        case t81::tisc::StructuralKind::TypeAlias: return "Alias";
        case t81::tisc::StructuralKind::Record:    return "Record";
        case t81::tisc::StructuralKind::Enum:      return "Enum";
    }
    return "Unknown";
}

std::string format_structural_alias(const t81::tisc::TypeAliasMetadata& alias) {
    std::string module = alias.module_path.empty() ? "<source>" : alias.module_path;
    std::ostringstream oss;
    oss << alias.name << " [" << structural_kind_name(alias.kind) << "]"
        << " schema=" << alias.schema_version
        << " module=" << module;
    if (!alias.fields.empty()) {
        oss << " fields=";
        for (size_t i = 0; i < alias.fields.size(); ++i) {
            if (i) oss << ',';
            oss << alias.fields[i].name;
        }
    }
    if (!alias.variants.empty()) {
        oss << " variants=";
        for (size_t i = 0; i < alias.variants.size(); ++i) {
            if (i) oss << ',';
            oss << alias.variants[i].name;
        }
    }
    return oss.str();
}

std::string to_lower(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

std::string trim_copy(std::string_view text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return std::string(text.substr(start, end - start));
}

std::vector<std::string> split_lines(std::string_view content) {
    std::string text(content);
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string summarize_snippet(const std::string& snippet) {
    std::string summary = snippet;
    auto newline = summary.find('\n');
    if (newline != std::string::npos) {
        summary = summary.substr(0, newline);
    }
    if (summary.size() > 64) {
        summary = summary.substr(0, 61) + "...";
    }
    return summary;
}

std::string opcode_name(t81::tisc::Opcode opcode) {
    switch (opcode) {
#define CASE(name) case t81::tisc::Opcode::name: return #name;
        CASE(Nop)
        CASE(Halt)
        CASE(LoadImm)
        CASE(Load)
        CASE(Store)
        CASE(Add)
        CASE(Sub)
        CASE(Mul)
        CASE(Div)
        CASE(Mod)
        CASE(Jump)
        CASE(JumpIfZero)
        CASE(Mov)
        CASE(Inc)
        CASE(Dec)
        CASE(Cmp)
        CASE(Push)
        CASE(Pop)
        CASE(TNot)
        CASE(TAnd)
        CASE(TOr)
        CASE(TXor)
        CASE(AxRead)
        CASE(AxSet)
        CASE(AxVerify)
        CASE(JumpIfNotZero)
        CASE(Call)
        CASE(Ret)
        CASE(Trap)
        CASE(I2F)
        CASE(F2I)
        CASE(I2Frac)
        CASE(Frac2I)
        CASE(TVecAdd)
        CASE(TMatMul)
        CASE(TTenDot)
        CASE(FAdd)
        CASE(FSub)
        CASE(FMul)
        CASE(FDiv)
        CASE(FracAdd)
        CASE(FracSub)
        CASE(FracMul)
        CASE(FracDiv)
        CASE(SetF)
        CASE(ChkShape)
        CASE(MakeOptionSome)
        CASE(MakeOptionNone)
        CASE(MakeResultOk)
        CASE(MakeResultErr)
        CASE(MakeEnumVariant)
        CASE(MakeEnumVariantPayload)
        CASE(OptionIsSome)
        CASE(OptionUnwrap)
        CASE(ResultIsOk)
        CASE(ResultUnwrapOk)
        CASE(ResultUnwrapErr)
        CASE(EnumIsVariant)
        CASE(EnumUnwrapPayload)
        CASE(Neg)
        CASE(JumpIfNegative)
        CASE(JumpIfPositive)
        CASE(Less)
        CASE(LessEqual)
        CASE(Greater)
        CASE(GreaterEqual)
        CASE(Equal)
        CASE(NotEqual)
        CASE(StackAlloc)
        CASE(StackFree)
        CASE(HeapAlloc)
        CASE(HeapFree)
        CASE(WeightsLoad)
#undef CASE
    }
    return "Opcode(" + std::to_string(static_cast<int>(opcode)) + ")";
}

std::string format_trace_entry(const t81::vm::TraceEntry& entry) {
    std::ostringstream oss;
    oss << "PC=" << entry.pc << ' ' << opcode_name(entry.opcode);
    if (entry.trap) {
        oss << " trap=" << t81::vm::to_string(*entry.trap);
    }
    return oss.str();
}

void print_trace_summary(const t81::vm::State& state) {
    const auto& trace = state.trace;
    if (trace.empty()) {
        info("Trace is empty");
        return;
    }
    info("Last trace entries:");
    size_t limit = std::min<std::size_t>(trace.size(), 16);
    for (size_t i = 0; i < limit; ++i) {
        std::string entry = format_trace_entry(trace[i]);
        info("  " + std::to_string(i + 1) + ": " + entry);
    }
    if (trace.size() > limit) {
        info("  ... " + std::to_string(trace.size() - limit) + " more entries");
    }
}

void print_bindings_summary(const t81::vm::State& state) {
    if (state.symbols.empty()) {
        info("No symbols recorded from the last run");
        return;
    }
    info("Symbols from last run:");
    size_t limit = std::min<std::size_t>(state.symbols.size(), 16);
    for (size_t i = 0; i < limit; ++i) {
        info("  " + std::to_string(i + 1) + ": " + state.symbols[i]);
    }
    if (state.symbols.size() > limit) {
        info("  ... " + std::to_string(state.symbols.size() - limit) + " more symbols");
    }
}

bool load_weights_model_from_path(const fs::path& path,
                                  std::shared_ptr<t81::weights::ModelFile>& model,
                                  std::optional<fs::path>& model_path,
                                  std::string& error) {
    if (!fs::exists(path)) {
        error = "weights model file not found";
        return false;
    }
    std::string ext = to_lower(path.extension().string());
    try {
        t81::weights::ModelFile loaded;
        if (ext == ".gguf") {
            loaded = t81::weights::load_gguf(path);
        } else if (ext == ".safetensors" || ext == ".safetensor") {
            loaded = t81::weights::load_safetensors(path);
        } else if (ext == ".t81w") {
            loaded = t81::weights::load_t81w(path);
        } else {
            error = "unsupported weights extension '" + ext + "'";
            return false;
        }
        model = std::make_shared<t81::weights::ModelFile>(std::move(loaded));
        model_path = path;
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

} // namespace

std::string sanitize_symbol(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    if (out.empty()) out = "_";
    return out;
}

std::string escape_metadata_string(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

std::string format_loop_metadata(const std::vector<t81::frontend::SemanticAnalyzer::LoopMetadata>& loops) {
  if (loops.empty()) return {};
  std::ostringstream oss;
  oss << "(policy (tier 1)";
    for (const auto& meta : loops) {
        const std::string file = meta.source_file.empty() ? "<source>" : meta.source_file;
        oss << " (loop"
            << " (id " << meta.id << ")"
            << " (file " << sanitize_symbol(file) << ")"
            << " (line " << meta.keyword.line << ")"
            << " (column " << meta.keyword.column << ")"
            << " (annotated " << (meta.annotated() ? "true" : "false") << ")"
            << " (depth " << meta.depth << ")";
        oss << " (guard " << (meta.guard_present ? "true" : "false") << ")";
        oss << " (bound ";
        using t81::frontend::LoopStmt;
        if (meta.bound_kind == LoopStmt::BoundKind::Infinite) {
            oss << "infinite";
        } else if (meta.bound_kind == LoopStmt::BoundKind::Static && meta.bound_value) {
            oss << *meta.bound_value;
        } else if (meta.bound_kind == LoopStmt::BoundKind::Guarded) {
            oss << "guarded";
        } else {
            oss << "unknown";
        }
        oss << ")";
        oss << ")";
    }
  oss << ")";
  return oss.str();
}

std::string pattern_kind_name(t81::frontend::MatchPattern::Kind kind) {
    switch (kind) {
        case t81::frontend::MatchPattern::Kind::Identifier: return "Identifier";
        case t81::frontend::MatchPattern::Kind::Tuple: return "Tuple";
        case t81::frontend::MatchPattern::Kind::Record: return "Record";
        case t81::frontend::MatchPattern::Kind::Variant: return "Variant";
        case t81::frontend::MatchPattern::Kind::None: return "None";
    }
    return "UnknownPattern";
}

std::string match_kind_name(t81::frontend::SemanticAnalyzer::MatchMetadata::Kind kind) {
    using MatchKind = t81::frontend::SemanticAnalyzer::MatchMetadata::Kind;
    switch (kind) {
        case MatchKind::Option: return "Option";
        case MatchKind::Result: return "Result";
        case MatchKind::Enum: return "Enum";
        default: return "Unknown";
    }
}

std::string format_match_metadata(const t81::frontend::SemanticAnalyzer& analyzer) {
    const auto& matches = analyzer.match_metadata();
    if (matches.empty()) return {};
    std::ostringstream oss;
    oss << "(match-metadata";
    for (const auto& meta : matches) {
        oss << " (match";
        oss << " (scrutinee " << match_kind_name(meta.kind) << ")";
        oss << " (type " << sanitize_symbol(analyzer.type_name(meta.result_type)) << ")";
        oss << " (guards " << (meta.guard_present ? "true" : "false") << ")";
        if (!meta.arms.empty()) {
            oss << " (arms";
            for (const auto& arm : meta.arms) {
                oss << " (arm";
                oss << " (variant " << sanitize_symbol(arm.variant) << ")";
                if (arm.variant_id >= 0) {
                    oss << " (variant-id " << arm.variant_id << ")";
                }
                oss << " (pattern " << pattern_kind_name(arm.pattern_kind) << ")";
                oss << " (guard " << (arm.has_guard ? "true" : "false") << ")";
                if (arm.payload_type.kind != t81::frontend::Type::Kind::Unknown) {
                    oss << " (payload " << sanitize_symbol(analyzer.type_name(arm.payload_type)) << ")";
                }
                if (arm.arm_type.kind != t81::frontend::Type::Kind::Unknown) {
                    oss << " (type " << sanitize_symbol(analyzer.type_name(arm.arm_type)) << ")";
                }
                if (arm.has_guard && !arm.guard_expression.empty()) {
                    oss << " (guard-expr \"" << escape_metadata_string(arm.guard_expression) << "\")";
                }
                oss << ")";
            }
            oss << ")";
        }
        oss << ")";
    }
    oss << ")";
    return oss.str();
}

std::vector<t81::tisc::EnumMetadata> collect_enum_metadata(const t81::frontend::SemanticAnalyzer& analyzer) {
    std::vector<t81::tisc::EnumMetadata> enums;
    const auto& definitions = analyzer.enum_definitions();
    enums.reserve(definitions.size());
    for (const auto& [name, info] : definitions) {
        if (info.id < 0) {
            continue;
        }
        t81::tisc::EnumMetadata entry;
        entry.enum_id = info.id;
        entry.name = name;
        entry.variants.reserve(info.variant_order.size());
        for (const auto& variant_name : info.variant_order) {
            auto variant_it = info.variants.find(variant_name);
            if (variant_it == info.variants.end()) {
                continue;
            }
            t81::tisc::EnumVariantMetadata variant_meta;
            variant_meta.name = variant_name;
            variant_meta.variant_id = variant_it->second.id;
            if (variant_it->second.payload.has_value()) {
                variant_meta.payload = analyzer.type_name(*variant_it->second.payload);
            }
            entry.variants.push_back(std::move(variant_meta));
        }
        enums.push_back(std::move(entry));
    }
    return enums;
}

namespace t81::cli {

std::optional<t81::tisc::Program> build_program_from_source(
    const std::string& source,
    const std::string& diag_name,
    const std::shared_ptr<t81::weights::ModelFile>& weights_model)
{
    verbose("Lexing...");
    t81::frontend::Lexer lexer(source);
    auto tokens = lexer.all_tokens();

    bool lexer_error = false;
    for (const auto& t : tokens) {
        if (t.type == t81::frontend::TokenType::Illegal) {
            lexer_error = true;
            std::cerr << diag_name << ':' << t.line << ':' << t.column
                      << ": illegal token `" << t.lexeme << "`\n";
        }
    }
    if (lexer_error) {
        error("Lexing failed");
        return std::nullopt;
    }

    verbose("Parsing...");
    t81::frontend::Lexer parser_lexer(source);
    t81::frontend::Parser parser(parser_lexer, diag_name);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        error("Parse errors encountered");
        return std::nullopt;
    }

    verbose("Semantic analysis...");
    t81::frontend::SemanticAnalyzer semantic_analyzer(stmts, diag_name);
    semantic_analyzer.analyze();
    if (semantic_analyzer.had_error()) {
        print_semantic_diagnostics(semantic_analyzer, diag_name, &source);
        error("Semantic errors encountered");
        return std::nullopt;
    }

    verbose("Generating IR...");
    t81::frontend::IRGenerator ir_gen;
    ir_gen.attach_semantic_analyzer(&semantic_analyzer);
    auto ir = ir_gen.generate(stmts);

    verbose("Emitting TISC bytecode...");
    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir);

    auto loop_policy = format_loop_metadata(semantic_analyzer.loop_metadata());
    if (!loop_policy.empty()) {
        program.axion_policy_text = loop_policy;
        verbose("Axion loop metadata emitted");
    }

    auto match_policy = format_match_metadata(semantic_analyzer);
    if (!match_policy.empty()) {
        program.match_metadata_text = match_policy;
        verbose("Match metadata emitted");
        verbose(match_policy);
    }

    auto enum_metadata = collect_enum_metadata(semantic_analyzer);
    if (!enum_metadata.empty()) {
        program.enum_metadata = std::move(enum_metadata);
        verbose("Enum metadata emitted");
    }

    if (weights_model) {
        program.weights_model = weights_model;
    }

    if (!program.type_aliases.empty()) {
        std::ostringstream oss;
        oss << "Structural metadata (" << program.type_aliases.size() << " entries):";
        for (const auto& alias : program.type_aliases) {
            oss << "\n  " << format_structural_alias(alias);
        }
        verbose(oss.str());
    }

    return program;
}

int compile(const fs::path& input,
            const fs::path& output,
            const std::string& source_override,
            const std::string& source_name,
            std::shared_ptr<t81::weights::ModelFile> weights_model) {
    verbose("Compiling " + input.string() + " → " + output.string());

    std::string diag_name = source_name.empty() ? input.string() : source_name;
    std::string source;
    if (source_override.empty()) {
        if (!fs::exists(input)) {
            error("Input file not found: " + input.string());
            return 1;
        }

        source = [] (const fs::path& p) {
            std::ifstream f(p, std::ios::binary);
            if (!f) throw std::runtime_error("Failed to open source file");
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }(input);
    } else {
        source = source_override;
    }

    auto program = build_program_from_source(source, diag_name, weights_model);
    if (!program) return 1;

    verbose("Writing " + output.string());
    t81::tisc::save_program(*program, output.string());

    info("Compilation successful → " + output.string());
    verbose(std::to_string(program->insns.size()) + " instructions emitted");
    return 0;
}

namespace {

void print_repl_help() {
    info("REPL commands:");
    info("  :quit / :exit       Exit the interactive session");
    info("  :help               Show this message again");
    info("  :history            Dump the pending buffer and recent runs");
    info("  :reset              Clear the current buffer");
    info("  :load <path>        Load a file into the buffer");
    info("  :save <path>        Persist the buffer to disk");
    info("  :run                Force execution without an empty line");
    info("  :model [path]       Show/replace the attached weights model");
    info("  :verbose / :quiet   Toggle logging verbosity");
    info("  :bindings / :symbols List recorded symbols from the last run");
    info("  :trace / :trill     Dump the last VM trace");
    info("Submit an empty line to compile and execute the buffered snippet.");
}

void print_repl_history(const std::vector<std::string>& buffer_lines,
                        const std::vector<std::string>& history_snippets) {
    if (buffer_lines.empty()) {
        info("REPL buffer is empty");
    } else {
        info("REPL buffer:");
        for (size_t i = 0; i < buffer_lines.size(); ++i) {
            const std::string& line = buffer_lines[i];
            const std::string display = line.empty() ? "<empty>" : line;
            info("  " + std::to_string(i + 1) + ": " + display);
        }
    }

    if (history_snippets.empty()) {
        info("No previous executions");
        return;
    }
    info("Previous executions:");
    size_t limit = std::min<std::size_t>(history_snippets.size(), 5);
    for (size_t i = 0; i < limit; ++i) {
        info("  " + std::to_string(i + 1) + ": " + summarize_snippet(history_snippets[i]));
    }
    if (history_snippets.size() > limit) {
        info("  ... " + std::to_string(history_snippets.size() - limit) + " more entries");
    }
}

}  // namespace

int repl(const std::shared_ptr<t81::weights::ModelFile>& weights_model,
         std::istream& input) {
    info("Entering T81 interactive REPL. Type ':quit' or ':exit' to leave; submit an empty line to run.");
    std::string buffer;
    std::vector<std::string> buffer_lines;
    std::vector<std::string> executed_snippets;
    std::unique_ptr<t81::vm::IVirtualMachine> last_vm;
    std::shared_ptr<t81::weights::ModelFile> active_model = weights_model;
    std::optional<fs::path> attached_model_path;

    auto ensure_newline = [&]() {
        if (!buffer.empty() && buffer.back() != '\n') {
            buffer.push_back('\n');
        }
    };

    auto append_line = [&](const std::string& line) {
        ensure_newline();
        buffer += line;
        buffer.push_back('\n');
        buffer_lines.push_back(line);
    };

    auto clear_buffer = [&]() {
        buffer.clear();
        buffer_lines.clear();
    };

    constexpr size_t kHistoryLimit = 64;

    auto execute_buffer = [&]() -> bool {
        if (buffer.empty()) {
            info("Nothing to run (buffer is empty)");
            return true;
        }
        auto program = build_program_from_source(buffer, "<repl>", active_model);
        if (!program) {
            clear_buffer();
            return false;
        }
        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(*program);
        auto result = vm->run_to_halt();
        last_vm = std::move(vm);
        executed_snippets.push_back(buffer);
        if (executed_snippets.size() > kHistoryLimit) {
            executed_snippets.erase(executed_snippets.begin());
        }
        if (!result) {
            error("Execution trapped: " + t81::vm::to_string(result.error()));
            clear_buffer();
            return false;
        }
        info("Execution completed");
        clear_buffer();
        return true;
    };

    auto set_buffer_from_string = [&](std::string content) {
        buffer = std::move(content);
        buffer_lines = split_lines(buffer);
    };

    while (true) {
        std::cout << (buffer.empty() ? "t81> " : ".... ") << std::flush;
        std::string line;
        if (!std::getline(input, line)) {
            info("Exiting REPL");
            return 0;
        }

        std::string trimmed = trim_copy(line);
        if (!trimmed.empty() && trimmed.front() == ':') {
            std::istringstream command(trimmed);
            std::string token;
            command >> token;
            std::string args;
            std::getline(command, args);
            args = trim_copy(args);

            if (token == ":quit" || token == ":exit") {
                break;
            }
            if (token == ":help") {
                print_repl_help();
                continue;
            }
            if (token == ":history") {
                print_repl_history(buffer_lines, executed_snippets);
                continue;
            }
            if (token == ":reset") {
                if (buffer.empty()) {
                    info("REPL buffer already empty");
                } else {
                    clear_buffer();
                    info("REPL buffer cleared");
                }
                continue;
            }
            if (token == ":load") {
                if (args.empty()) {
                    error("Missing path for :load");
                    continue;
                }
                fs::path path(args);
                std::ifstream in(path);
                if (!in) {
                    error("Failed to open file: " + path.string());
                    continue;
                }
                std::ostringstream ss;
                ss << in.rdbuf();
                set_buffer_from_string(ss.str());
                info("Loaded snippet from " + path.string());
                continue;
            }
            if (token == ":save") {
                if (args.empty()) {
                    error("Missing path for :save");
                    continue;
                }
                fs::path path(args);
                std::ofstream out(path, std::ios::binary);
                if (!out) {
                    error("Failed to write file: " + path.string());
                    continue;
                }
                out << buffer;
                info("Buffer saved to " + path.string());
                continue;
            }
            if (token == ":run") {
                execute_buffer();
                continue;
            }
            if (token == ":model") {
                if (args.empty()) {
                    if (attached_model_path) {
                        info("Attached weights model: " + attached_model_path->string());
                    } else if (active_model) {
                        info("Attached weights model (path unknown)");
                    } else {
                        info("No weights model attached");
                    }
                    continue;
                }
                if (args == "none") {
                    active_model.reset();
                    attached_model_path.reset();
                    info("Weights model cleared");
                    continue;
                }
                std::string error_msg;
                fs::path path(args);
                if (!load_weights_model_from_path(path, active_model, attached_model_path, error_msg)) {
                    error("Failed to load model: " + error_msg);
                } else {
                    info("Loaded weights model from " + path.string());
                }
                continue;
            }
            if (token == ":verbose") {
                g_flags.verbose = true;
                g_flags.quiet = false;
                info("Verbose logging enabled");
                continue;
            }
            if (token == ":quiet") {
                g_flags.quiet = true;
                g_flags.verbose = false;
                info("Quiet mode enabled");
                continue;
            }
            if (token == ":bindings" || token == ":symbols") {
                if (last_vm) {
                    print_bindings_summary(last_vm->state());
                } else {
                    info("No execution recorded yet");
                }
                continue;
            }
            if (token == ":trace" || token == ":trill") {
                if (last_vm) {
                    print_trace_summary(last_vm->state());
                } else {
                    info("No trace available yet");
                }
                continue;
            }
            info("Unknown command: " + token);
            continue;
        }

        if (line.empty()) {
            if (buffer.empty()) {
                continue;
            }
            execute_buffer();
            continue;
        }

        append_line(line);
    }

    info("Exiting REPL");
    return 0;
}

int run_tisc(const fs::path& path) {
    verbose("Loading TISC program: " + path.string());

    auto program = t81::tisc::load_program(path.string());
    verbose("Program loaded (" + std::to_string(program.insns.size()) + " insns)");

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);

    verbose("Executing...");
    auto result = vm->run_to_halt();

    if (!result) {
        error("Execution trapped: " + t81::vm::to_string(result.error()));
        return trap_exit_code(result.error());
    }

    info("Program terminated normally");
    return 0;
}

int check_syntax(const fs::path& path) {
    verbose("Syntax-checking " + path.string());

    std::string source = [] (const fs::path& p) {
        std::ifstream f(p);
        if (!f) throw std::runtime_error("Cannot open file");
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }(path);

    auto program = build_program_from_source(source, path.string());
    if (!program) {
        return 1;
    }

    info("No syntax or semantic errors");
    return 0;
}

} // namespace t81::cli
