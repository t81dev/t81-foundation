#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <iostream>

void test_option_result_flow() {
    std::string source = R"(
        fn make_option() -> Option[i32] {
            let value: Option[i32] = Some(123);
            return value;
        }

        fn make_result_inferred() -> Result[i32, T81String] {
            let inferred = Ok(7);
            return inferred;
        }

        fn make_result_error() -> Result[i32, T81String] {
            let inferred = Err("boom");
            return inferred;
        }

        fn main() -> i32 {
            let opt: Option[i32] = make_option();
            let good: Result[i32, T81String] = make_result_inferred();
            let bad: Result[i32, T81String] = make_result_error();
            return 55;
        }
    )";

    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();

    assert(!parser.had_error() && "Parsing failed");

    t81::frontend::SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(!analyzer.had_error() && "Semantic analysis failed");

    t81::frontend::IRGenerator generator;
    auto ir_program = generator.generate(stmts);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    vm->run_to_halt();

    assert(vm->state().registers[0] == 55 && "VM register R0 has incorrect value for option/result flow");

    std::cout << "E2ETest test_option_result_flow passed!" << std::endl;
}

int main() {
    test_option_result_flow();
    return 0;
}
