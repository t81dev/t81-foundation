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

using namespace t81;

int64_t execute_e2e_option_result_function_test(const std::string& source) {
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

void test_option_result_function_regression() {
    const std::string source = R"(
        fn produce_payload(flag: i32) -> Option[Vector[i32]] {
            let template: Vector[i32] = [4, 8, 12];
            if (flag > 0) {
                return Some(template);
            }
            return None;
        }

        fn inspect_payload(value: Option[Vector[i32]]) -> Result[i32, T81String] {
            return match (value) {
                Some(_) => Ok(7);
                None => Err("missing payload");
            };
        }

        fn main() -> i32 {
            let present_result = inspect_payload(produce_payload(1));
            let absent_result = inspect_payload(produce_payload(0));

            let present_value: i32 = match (present_result) {
                Ok(v) => v;
                Err(_) => -1;
            };

            let absent_value: i32 = match (absent_result) {
                Ok(v) => v;
                Err(_) => 3;
            };

            return present_value + absent_value;
        }
    )";

    assert(execute_e2e_option_result_function_test(source) == 10);
}

int main() {
    test_option_result_function_regression();
    std::cout << "E2E option/result function regression passed!" << std::endl;
    return 0;
}
