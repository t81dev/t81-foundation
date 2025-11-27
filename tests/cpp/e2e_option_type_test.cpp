#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void test_option_type_e2e() {
    std::string source = R"(
        fn create_some() -> Option[i32] {
            let x: Option[i32] = Some(42);
            return x;
        }

        fn main() -> i32 {
            let maybe_val: Option[i32] = create_some();
            // TODO: Add a match expression here once they are supported.
            // For now, we will rely on a VM-level intrinsic or a
            // yet-to-be-implemented `unwrap` method.
            return 1; // Placeholder
        }
    )";

    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();

    assert(!parser.had_error() && "Parsing failed");

    t81::frontend::SemanticAnalyzer semantic_analyzer(stmts);
    semantic_analyzer.analyze();
    assert(!semantic_analyzer.had_error() && "Semantic analysis failed");

    t81::frontend::IRGenerator generator;
    auto ir_program = generator.generate(stmts);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    vm->run_to_halt();

    assert(vm->state().registers[0] == 1 && "VM register R0 has incorrect value");

    std::cout << "E2ETest test_option_type_e2e passed!" << std::endl;
}

int main() {
    test_option_type_e2e();
    return 0;
}
