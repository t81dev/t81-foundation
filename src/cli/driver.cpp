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

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
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
void print_semantic_diagnostics(const t81::frontend::SemanticAnalyzer& analyzer) {
    for (const auto& diag : analyzer.diagnostics()) {
        const std::string file = diag.file.empty() ? "<source>" : diag.file;
        std::cerr << file << ':' << diag.line << ':' << diag.column
                  << ": error: " << diag.message << '\n';
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
        oss << " (bound ";
        using t81::frontend::LoopStmt;
        if (meta.bound_kind == LoopStmt::BoundKind::Infinite) {
            oss << "infinite";
        } else if (meta.bound_kind == LoopStmt::BoundKind::Static && meta.bound_value) {
            oss << *meta.bound_value;
        } else {
            oss << "unknown";
        }
        oss << ")";
        oss << ")";
    }
    oss << ")";
    return oss.str();
}

std::optional<t81::tisc::Program> build_program_from_source(
    const std::string& source,
    const std::string& diag_name,
    const std::shared_ptr<t81::weights::ModelFile>& weights_model = nullptr)
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
        print_semantic_diagnostics(semantic_analyzer);
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

namespace t81::cli {

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

int repl(const std::shared_ptr<t81::weights::ModelFile>& weights_model,
         std::istream& input) {
    info("Entering T81 interactive REPL. Type ':quit' or ':exit' to leave; submit an empty line to run.");
    std::string buffer;

    while (true) {
        std::cout << (buffer.empty() ? "t81> " : ".... ") << std::flush;
        std::string line;
        if (!std::getline(input, line)) {
            info("Exiting REPL");
            return 0;
        }
        if (line == ":quit" || line == ":exit") {
            break;
        }
        if (line.empty()) {
            if (buffer.empty()) {
                continue;
            }
            auto program = build_program_from_source(buffer, "<repl>", weights_model);
            if (program) {
                auto vm = t81::vm::make_interpreter_vm();
                vm->load_program(*program);
                auto result = vm->run_to_halt();
                if (!result) {
                    error("Execution trapped: " + t81::vm::to_string(result.error()));
                } else {
                    info("Execution completed");
                }
            }
            buffer.clear();
            continue;
        }
        buffer += line;
        buffer.push_back('\n');
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

    t81::frontend::Lexer lexer(source);
    auto tokens = lexer.all_tokens();

    bool ok = true;
    for (const auto& t : tokens) {
        if (t.type == t81::frontend::TokenType::Illegal) {
            ok = false;
            std::cerr << path.string() << ':' << t.line << ':' << t.column
                      << ": illegal token `" << t.lexeme << "`\n";
        }
    }
    if (!ok) {
        error("Lexing failed");
        return 1;
    }

    t81::frontend::Parser parser(lexer, path.string());
    parser.parse();
    if (parser.had_error()) {
        error("Syntax errors found");
        return 1;
    }

    info("No syntax errors");
    return 0;
}

} // namespace t81::cli
