#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include "t81/vm/state.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace t81;

// Helper to compile and run a T81Lang source string and return the final value of register r0.
int64_t run_e2e_test(const std::string& source) {
    frontend::Lexer lexer(source);
    frontend::Parser parser(lexer);
    auto stmts = parser.parse();
    assert(!parser.had_error());

    frontend::SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(!analyzer.had_error());

    frontend::IRGenerator ir_gen;
    ir_gen.attach_semantic_analyzer(&analyzer);
    tisc::ir::IntermediateProgram ir = ir_gen.generate(stmts);

    tisc::BinaryEmitter emitter;
    tisc::Program program = emitter.emit(ir);

    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    vm->run_to_halt();

    return vm->state().registers[0];
}

int main() {
    const std::string match_test_source = R"(
        fn main() -> i32 {
            let opt: Option[i32] = Some(123);
            let result: i32 = match (opt) {
                Some(value) => value,
                None => 42
            };
            return result;
        }
    )";

    int64_t result = run_e2e_test(match_test_source);
    assert(result == 123);

    std::cout << "E2E match expression test passed!" << std::endl;
    return 0;
}
