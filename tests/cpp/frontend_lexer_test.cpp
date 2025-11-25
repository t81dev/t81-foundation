#include "t81/frontend/lexer.hpp"
#include <vector>
#include <cassert>
#include <iostream>

using namespace t81::frontend;

struct ExpectedToken {
    TokenType type;
    const char* lexeme;
    int line;
    int column;
};

void test_sequence(const char* source, const std::vector<ExpectedToken>& expected_tokens) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.all_tokens();
    assert(tokens.size() == expected_tokens.size() + 1); // +1 for EOF

    for (size_t i = 0; i < expected_tokens.size(); ++i) {
        const auto& actual = tokens[i];
        const auto& expected = expected_tokens[i];
        if (actual.type != expected.type || actual.lexeme != expected.lexeme || actual.line != expected.line || actual.column != expected.column) {
            std::cerr << "Test failed at token index " << i << std::endl;
            std::cerr << "  Expected: Type=" << static_cast<int>(expected.type) << ", Lexeme='" << expected.lexeme << "', Line=" << expected.line << ", Col=" << expected.column << std::endl;
            std::cerr << "  Actual:   Type=" << static_cast<int>(actual.type) << ", Lexeme='" << actual.lexeme << "', Line=" << actual.line << ", Col=" << actual.column << std::endl;
        }
        assert(actual.type == expected.type);
        assert(actual.lexeme == expected.lexeme);
        assert(actual.line == expected.line);
        assert(actual.column == expected.column);
    }
    assert(tokens.back().type == TokenType::Eof);
}

int main() {
    // Test a sequence of tokens with line and column numbers
    const char* source = R"(module my_mod;

fn main() -> i32 {
    let x = 1;
    return x;
}
)";
    std::vector<ExpectedToken> expected = {
        {TokenType::Module, "module", 1, 1},
        {TokenType::Identifier, "my_mod", 1, 8},
        {TokenType::Semicolon, ";", 1, 14},
        {TokenType::Fn, "fn", 3, 1},
        {TokenType::Identifier, "main", 3, 4},
        {TokenType::LParen, "(", 3, 8},
        {TokenType::RParen, ")", 3, 9},
        {TokenType::Arrow, "->", 3, 11},
        {TokenType::I32, "i32", 3, 14},
        {TokenType::LBrace, "{", 3, 18},
        {TokenType::Let, "let", 4, 5},
        {TokenType::Identifier, "x", 4, 9},
        {TokenType::Equal, "=", 4, 11},
        {TokenType::Integer, "1", 4, 13},
        {TokenType::Semicolon, ";", 4, 14},
        {TokenType::Return, "return", 5, 5},
        {TokenType::Identifier, "x", 5, 12},
        {TokenType::Semicolon, ";", 5, 13},
        {TokenType::RBrace, "}", 6, 1},
    };
    test_sequence(source, expected);

    std::cout << "All lexer tests passed!" << std::endl;

    return 0;
}
