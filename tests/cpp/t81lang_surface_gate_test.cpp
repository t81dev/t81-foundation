#include <iostream>
#include <vector>
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "test_runtime_check.hpp"

using namespace t81::frontend;

void check_no_errors(const std::string& src) {
    Lexer lexer(src);
    Parser parser(lexer, "gate_test");
    auto ast = parser.parse();
    T81_TEST_CHECK(!parser.had_error());

    SemanticAnalyzer sem(ast);
    sem.analyze();
    if (sem.had_error()) {
        for (auto err : sem.diagnostics()) {
            std::cerr << err.message << "\n";
        }
    }
    T81_TEST_CHECK(!sem.had_error());
}

int main() {
    std::cout << "Running t81lang_surface_gate tests...\n";

    // Types exposed: T81Quaternion, T81Prob, Cell
    check_no_errors("let q: T81Quaternion = T81Quaternion();");
    check_no_errors("let p: T81Prob = T81Prob();");
    check_no_errors("let c: Cell = Cell();");

    // New serializations/types: T81List, T81Set, T81Tree, T81Map, T81Graph
    check_no_errors("let l: List[i32] = List[i32]();");
    check_no_errors("let m: Map[i32, i32] = Map[i32, i32]();");
    check_no_errors("let s: Set[i32] = Set[i32]();");
    check_no_errors("let t: Tree[i32] = Tree[i32]();");

    // Match monadic stability
    check_no_errors("let opt: Option[i32] = Some(1); let val = match(opt) { Some(v) => v, None => 2.5 };");

    std::cout << "t81lang_surface_gate tests passed!\n";
    return 0;
}
