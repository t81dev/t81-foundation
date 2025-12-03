/**
 * @file lexer.hpp
 * @brief Defines the Lexer and token types for the T81Lang frontend.
 */

#ifndef T81_FRONTEND_LEXER_HPP
#define T81_FRONTEND_LEXER_HPP

#include <string>
#include <string_view>
#include <vector>

namespace t81 {
namespace frontend {

/**
 * @enum TokenType
 * @brief Describes the different types of tokens that the Lexer can produce.
 */
enum class TokenType {
    // Keywords
    Module, Type, Const, Export, Fn, Let, Var,
    Record, Enum,
    If, Else, For, In, While, Loop, Break, Continue, Return, Match,
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
    FatArrow,   // =>
    DotDot,     // ..
    Dot,        // .

    // Special
    At,         // @

    // Control
    Eof,        ///< End of file/source.
    Illegal     ///< An illegal or unexpected character.
};

/**
 * @struct Token
 * @brief Represents a single token scanned from the source code.
 */
struct Token {
    TokenType type;             ///< The type of the token.
    std::string_view lexeme;    ///< The substring from the source code.
    int line;                   ///< The line number where the token appears.
    int column;                 ///< The column number where the token begins.
};

/**
 * @class Lexer
 * @brief A lexical analyzer for the T81Lang language.
 *
 * The Lexer scans a source string and converts it into a sequence of tokens.
 */
class Lexer {
public:
    /**
     * @brief Constructs a Lexer for the given source code.
     * @param source A string_view of the source code to tokenize.
     */
    Lexer(std::string_view source);

    /**
     * @brief Scans and returns the next token in the source stream.
     * @return The next Token.
     */
    Token next_token();

    /**
     * @brief Scans the entire source and returns all tokens.
     * @return A vector containing all tokens from the source.
     */
    std::vector<Token> all_tokens();

    /**
     * @brief Peeks the next token without advancing the lexer state.
     * @return The next Token.
     */
    Token peek_next_token();

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
