/**
 * @file main.cpp
 * @brief T81 Foundation Command-Line Interface
 *
 * This file implements the `t81` CLI tool, providing commands for compiling,
 * running, and checking T81Lang source files.
 */

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/tisc/binary_io.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

namespace fs = std::filesystem;

// Version information
constexpr const char* T81_VERSION = "1.0.0-SOVEREIGN";

// Global flags
bool g_verbose = false;
bool g_quiet = false;

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

// Helper to read a file into a string
std::string read_file_to_string(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to write a string to a file
void write_string_to_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not write to file: " + path);
    }
    file << content;
}

// Helper to determine output path from input path
std::string default_output_path(const std::string& input_path, const std::string& extension) {
    fs::path input(input_path);
    fs::path output = input.parent_path() / (input.stem().string() + extension);
    return output.string();
}

// Verbose output helper
void verbose(const std::string& message) {
    if (g_verbose) {
        std::cerr << "[verbose] " << message << std::endl;
    }
}

// Error output helper
void error(const std::string& message) {
    if (!g_quiet) {
        std::cerr << "error: " << message << std::endl;
    }
}

// Info output helper
void info(const std::string& message) {
    if (!g_quiet) {
        std::cout << message << std::endl;
    }
}

// Print usage information
void print_usage(const char* program_name) {
    std::cerr << "T81 Foundation - Ternary-Native Computing Stack\n"
              << "Version " << T81_VERSION << "\n\n"
              << "Usage: " << program_name << " <command> [options] [arguments]\n\n"
              << "Commands:\n"
              << "  compile <input.t81> [-o <output.tisc>]  Compile a T81Lang source file to TISC bytecode\n"
              << "  run <input.tisc|input.t81>               Execute a TISC binary file or compile and run a .t81 file\n"
              << "  check <input.t81>                       Check the syntax of a T81Lang source file\n"
              << "  version                                  Show version information\n"
              << "  help                                    Show this help message\n\n"
              << "Options:\n"
              << "  -v, --verbose                           Enable verbose output\n"
              << "  -q, --quiet                             Suppress non-error output\n"
              << "  -h, --help                              Show help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " compile hello.t81 -o hello.tisc\n"
              << "  " << program_name << " run hello.tisc\n"
              << "  " << program_name << " check hello.t81\n"
              << std::endl;
}

// Print version information
void print_version() {
    std::cout << "T81 Foundation " << T81_VERSION << "\n"
              << "Ternary-Native Computing Stack\n"
              << "Copyright (c) 2025 T81 Foundation\n"
              << "Licensed under MIT and GPL-3.0\n"
              << std::endl;
}

// Forward declarations for actions
int compile_file(const std::string& input_path, const std::string& output_path);
int run_file(const std::string& input_path);
int check_file(const std::string& input_path);
int run_source_file(const std::string& input_path);

// Parse command-line arguments
struct ParsedArgs {
    std::string command;
    std::vector<std::string> positional_args;
    std::string output_path;
    bool has_output = false;
};

ParsedArgs parse_args(int argc, char* argv[]) {
    ParsedArgs args;
    
    if (argc < 2) {
        throw std::runtime_error("No command specified");
    }
    
    args.command = argv[1];
    
    // Handle global flags first
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            g_verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            g_quiet = true;
        } else if (arg == "-h" || arg == "--help") {
            args.command = "help";
            return args;
        } else if (arg == "--version") {
            args.command = "version";
            return args;
        }
    }
    
    // Parse command-specific arguments
    if (args.command == "compile") {
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-o" || arg == "--output") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("Missing output file after -o");
                }
                args.output_path = argv[++i];
                args.has_output = true;
            } else if (arg[0] != '-') {
                args.positional_args.push_back(arg);
            }
        }
        if (args.positional_args.empty()) {
            throw std::runtime_error("compile: missing input file");
        }
    } else if (args.command == "run" || args.command == "check") {
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg[0] != '-') {
                args.positional_args.push_back(arg);
            }
        }
        if (args.positional_args.empty()) {
            throw std::runtime_error(args.command + ": missing input file");
        }
    }
    
    return args;
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            print_usage(argv[0]);
            return 1;
        }
        
        std::string command = argv[1];
        
        // Handle simple commands that don't need parsing
        if (command == "version" || command == "--version" || command == "-V") {
            print_version();
            return 0;
        }
        
        if (command == "help" || command == "--help" || command == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        
        // Parse arguments
        ParsedArgs args = parse_args(argc, argv);
        
        // Execute command
        if (args.command == "compile") {
            std::string input_path = args.positional_args[0];
            std::string output_path;
            
            if (args.has_output) {
                output_path = args.output_path;
            } else {
                output_path = default_output_path(input_path, ".tisc");
            }
            
            return compile_file(input_path, output_path);
            
        } else if (args.command == "run") {
            std::string input_path = args.positional_args[0];
            // Check if input is a .t81 file - if so, compile and run it
            if (input_path.ends_with(".t81")) {
                return run_source_file(input_path);
            } else {
                return run_file(input_path);
            }
            
        } else if (args.command == "check") {
            std::string input_path = args.positional_args[0];
            return check_file(input_path);
            
        } else {
            error("Unknown command: " + args.command);
            print_usage(argv[0]);
            return 1;
        }
        
    } catch (const std::exception& e) {
        error(e.what());
        if (!g_quiet) {
            std::cerr << "Run '" << (argc > 0 ? argv[0] : "t81") << " help' for usage information." << std::endl;
        }
        return 1;
    }
    
    return 0;
}

// --- Command Implementations ---

int compile_file(const std::string& input_path, const std::string& output_path) {
    verbose("Reading source file: " + input_path);
    
    if (!fs::exists(input_path)) {
        error("Input file does not exist: " + input_path);
        return 1;
    }
    
    std::string source = read_file_to_string(input_path);
    verbose("Source file size: " + std::to_string(source.size()) + " bytes");
    
    verbose("Lexing source code...");
    t81::frontend::Lexer lexer(source);
    
    // Check for lexer errors
    auto tokens = lexer.all_tokens();
    bool has_lexer_errors = false;
    for (const auto& token : tokens) {
        if (token.type == t81::frontend::TokenType::Illegal) {
            has_lexer_errors = true;
            std::cerr << "Lexer Error at line " << token.line 
                      << ", column " << token.column << ": " 
                      << std::string(token.lexeme) << std::endl;
        }
    }
    
    if (has_lexer_errors) {
        error("Lexing failed");
        return 1;
    }
    
    verbose("Parsing tokens...");
    t81::frontend::Parser parser(lexer);
    auto statements = parser.parse();
    
    if (parser.had_error()) {
        error("Parsing failed");
        return 1;
    }
    
    verbose("Generating IR...");
    t81::frontend::IRGenerator ir_generator;
    auto ir_program = ir_generator.generate(statements);
    
    verbose("Emitting TISC bytecode...");
    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);
    
    verbose("Writing output to: " + output_path);
    t81::tisc::save_program(program, output_path);
    
    info("Compilation successful: " + input_path + " -> " + output_path);
    verbose("Generated " + std::to_string(program.insns.size()) + " instructions");
    
    return 0;
}

int run_file(const std::string& input_path) {
    verbose("Loading TISC program: " + input_path);
    
    if (!fs::exists(input_path)) {
        error("Input file does not exist: " + input_path);
        return 1;
    }
    
    auto program = t81::tisc::load_program(input_path);
    verbose("Loaded program with " + std::to_string(program.insns.size()) + " instructions");
    
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    
    verbose("Executing program...");
    auto result = vm->run_to_halt();
    
    if (!result) {
        error("VM execution failed with trap: " + t81::vm::to_string(result.error()));
        return 1;
    }
    
    info("Execution completed successfully");
    
    return 0;
}

int check_file(const std::string& input_path) {
    verbose("Checking syntax of: " + input_path);
    
    if (!fs::exists(input_path)) {
        error("Input file does not exist: " + input_path);
        return 1;
    }
    
    std::string source = read_file_to_string(input_path);
    
    verbose("Lexing source code...");
    t81::frontend::Lexer lexer(source);
    
    // Check for lexer errors
    auto tokens = lexer.all_tokens();
    bool has_lexer_errors = false;
    for (const auto& token : tokens) {
        if (token.type == t81::frontend::TokenType::Illegal) {
            has_lexer_errors = true;
            std::cerr << "Lexer Error at line " << token.line 
                      << ", column " << token.column << ": " 
                      << std::string(token.lexeme) << std::endl;
        }
    }
    
    if (has_lexer_errors) {
        error("Lexing failed");
        return 1;
    }
    
    verbose("Parsing tokens...");
    t81::frontend::Parser parser(lexer);
    parser.parse();
    
    if (parser.had_error()) {
        error("Syntax errors found");
        return 1;
    }
    
    info("No syntax errors found");
    return 0;
}

int run_source_file(const std::string& input_path) {
    // Compile the source file to a temporary location
    std::string temp_output = fs::temp_directory_path() / (fs::path(input_path).stem().string() + ".tisc");
    verbose("Compiling " + input_path + " to temporary file: " + temp_output);
    
    int compile_result = compile_file(input_path, temp_output);
    if (compile_result != 0) {
        return compile_result;
    }
    
    // Run the compiled file
    verbose("Running compiled program...");
    int run_result = run_file(temp_output);
    
    // Clean up temporary file
    if (fs::exists(temp_output)) {
        verbose("Cleaning up temporary file: " + temp_output);
        fs::remove(temp_output);
    }
    
    return run_result;
}
