#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>

#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/tisc/binary_io.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

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


// Forward declarations for actions
void compile_file(const std::string& input_path, const std::string& output_path);
void run_file(const std::string& input_path);
void check_file(const std::string& input_path);

void print_usage() {
    std::cerr << "Usage: t81 <command> [options]" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  compile <input.t81> -o <output.tisc>  Compile a T81Lang source file." << std::endl;
    std::cerr << "  run <input.tisc>                     Execute a TISC binary file." << std::endl;
    std::cerr << "  check <input.t81>                    Check the syntax of a T81Lang source file." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        if (command == "compile") {
            if (args.size() != 3 || args[1] != "-o") {
                std::cerr << "Error: Invalid arguments for compile command." << std::endl;
                print_usage();
                return 1;
            }
            std::string input_path = args[0];
            std::string output_path = args[2];
            compile_file(input_path, output_path);
        } else if (command == "run") {
            if (args.size() != 1) {
                std::cerr << "Error: Invalid arguments for run command." << std::endl;
                print_usage();
                return 1;
            }
            std::string input_path = args[0];
            run_file(input_path);
        } else if (command == "check") {
            if (args.size() != 1) {
                std::cerr << "Error: Invalid arguments for check command." << std::endl;
                print_usage();
                return 1;
            }
            std::string input_path = args[0];
            check_file(input_path);
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            print_usage();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// --- Implementations ---

void compile_file(const std::string& input_path, const std::string& output_path) {
    std::cout << "Compiling " << input_path << " to " << output_path << std::endl;

    std::string source = read_file_to_string(input_path);
    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto statements = parser.parse();

    if (parser.had_error()) {
        throw std::runtime_error("Parsing failed.");
    }

    t81::frontend::IRGenerator ir_generator;
    auto ir_program = ir_generator.generate(statements);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    t81::tisc::save_program(program, output_path);
    std::cout << "Compilation successful." << std::endl;
}

void run_file(const std::string& input_path) {
    std::cout << "Running " << input_path << std::endl;

    auto program = t81::tisc::load_program(input_path);

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();

    if (!result) {
        throw std::runtime_error("VM execution failed with trap: " + to_string(result.error()));
    }

    std::cout << "Execution successful." << std::endl;
}

void check_file(const std::string& input_path) {
    std::cout << "Checking " << input_path << std::endl;
    std::string source = read_file_to_string(input_path);
    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    parser.parse();

    if (parser.had_error()) {
        std::cout << "Syntax errors found." << std::endl;
    } else {
        std::cout << "No syntax errors found." << std::endl;
    }
}
