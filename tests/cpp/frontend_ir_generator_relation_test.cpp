#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/tisc/ir.hpp"

#include <iostream>

using namespace t81::frontend;
using namespace t81::tisc::ir;

void test_relation_metadata() {
    std::string source = "let cmp = 1 < 2;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();

    IRGenerator generator;
    generator.attach_semantic_analyzer(&analyzer);
    auto program = generator.generate(stmts);

    bool found = false;
    for (const auto& inst : program.instructions()) {
        if (inst.opcode == Opcode::CMP &&
            inst.boolean_result &&
            inst.relation == ComparisonRelation::Less) {
            found = true;
            break;
        }
    }
    assert(found && "Expected CMP instruction tagged as Less boolean.");
    std::cout << "FrontendIRGeneratorRelationTest passed!" << std::endl;
}

int main() {
    test_relation_metadata();
    return 0;
}
