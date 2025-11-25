#ifndef T81_FRONTEND_LEXER_HPP
#define T81_FRONTEND_LEXER_HPP

#include <string>
#include <string_view>
#include <vector>

namespace t81 {
namespace frontend {

enum class TokenType {
    // Keywords
    Module, Type, Const, Export, Fn, Let, Var,
    If, Else, For, In, While, Break, Continue, Return,
    True, False,

    // Type Keywords
    Void, Bool, I32, I16, I8, I2,
    T81BigInt, T81Float, T81Fraction,
    Vector, Matrix, Tensor, Graph,

    // Literals
    Integer,
    Float,
    String,
    Ternary,
    Base81Integer,
    Base81Float,

    // Identifier
    Identifier,

    // Operators
    Plus, Minus, Star, Slash, Percent,
    Equal, EqualEqual, Bang, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,
    Amp, AmpAmp, Pipe, PipePipe, Caret,
    Question,

    // Punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Colon, Semicolon,
    Arrow,      // ->
    DotDot,     // ..

    // Special
    At,         // @

    // Control
    Eof,
    Illegal
};

struct Token {
    TokenType type;
    std::string_view lexeme;
    int line;
    int column;
};

class Lexer {
public:
    Lexer(std::string_view source);

    Token next_token();
    std::vector<Token> all_tokens();

private:
    char advance();
    char peek() const;
    char peek_next() const;
    bool is_at_end() const;

    Token make_token(TokenType type);
    Token error_token(const char* message);
    Token string();
    Token number();
    Token identifier();

    void skip_whitespace_and_comments();
    bool match(char expected);

    std::string_view _source;
    std::string_view::iterator _current;
    std::string_view::iterator _line_start;
    std::string_view::iterator _token_start;
    int _line;
};

} // namespace frontend
} // namespace t81

#endif // T81_FRONTEND_LEXER_HPP
