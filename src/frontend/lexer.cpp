/**
 * @file lexer.cpp
 * @brief Implements the Lexer for the T81Lang frontend.
 */

#include "t81/frontend/lexer.hpp"

#include <unordered_map>

namespace t81 {
namespace frontend {

namespace {

// A map of T81Lang keywords to their corresponding token types.
const std::unordered_map<std::string_view, TokenType> KEYWORDS = {
    {"module", TokenType::Module},
    {"type", TokenType::Type},
    {"const", TokenType::Const},
    {"export", TokenType::Export},
    {"fn", TokenType::Fn},
    {"let", TokenType::Let},
    {"var", TokenType::Var},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"for", TokenType::For},
    {"in", TokenType::In},
    {"while", TokenType::While},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"return", TokenType::Return},
    {"true", TokenType::True},
    {"false", TokenType::False},
    {"void", TokenType::Void},
    {"bool", TokenType::Bool},
    {"i32", TokenType::I32},
    {"i16", TokenType::I16},
    {"i8", TokenType::I8},
    {"i2", TokenType::I2},
    {"T81BigInt", TokenType::T81BigInt},
    {"T81Float", TokenType::T81Float},
    {"T81Fraction", TokenType::T81Fraction},
    {"vector", TokenType::Vector},
    {"matrix", TokenType::Matrix},
    {"tensor", TokenType::Tensor},
    {"graph", TokenType::Graph},
};

// Helper to check if a character is alphabetic or an underscore.
bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Helper to check if a character is a decimal digit.
bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

} // namespace

/**
 * @brief Constructs a new Lexer.
 * @param source The source code to be tokenized.
 */
Lexer::Lexer(std::string_view source)
    : _source(source),
      _current(_source.begin()),
      _line_start(_source.begin()),
      _token_start(_source.begin()),
      _line(1) {}

/**
 * @brief Scans and returns the next token from the source code.
 *
 * This is the main entry point of the lexer. It skips whitespace and comments,
 * then identifies and constructs the next token based on the current character.
 *
 * @return The next token in the stream. Returns TokenType::Eof at the end.
 */
Token Lexer::next_token() {
    skip_whitespace_and_comments();

    _token_start = _current;

    if (is_at_end()) return make_token(TokenType::Eof);

    char c = advance();

    if (is_alpha(c)) return identifier();
    if (is_digit(c)) return number();

    switch (c) {
        // Single-character tokens
        case '(': return make_token(TokenType::LParen);
        case ')': return make_token(TokenType::RParen);
        case '{': return make_token(TokenType::LBrace);
        case '}': return make_token(TokenType::RBrace);
        case '[': return make_token(TokenType::LBracket);
        case ']': return make_token(TokenType::RBracket);
        case ',': return make_token(TokenType::Comma);
        case ':': return make_token(TokenType::Colon);
        case ';': return make_token(TokenType::Semicolon);
        case '@': return make_token(TokenType::At);
        case '?': return make_token(TokenType::Question);
        case '+': return make_token(TokenType::Plus);
        case '*': return make_token(TokenType::Star);
        case '%': return make_token(TokenType::Percent);
        case '^': return make_token(TokenType::Caret);
        case '/': return make_token(TokenType::Slash);

        // One or two-character tokens
        case '-': return make_token(match('>') ? TokenType::Arrow : TokenType::Minus);
        case '.': return make_token(match('.') ? TokenType::DotDot : TokenType::Illegal);
        case '=': return make_token(match('=') ? TokenType::EqualEqual : TokenType::Equal);
        case '!': return make_token(match('=') ? TokenType::BangEqual : TokenType::Bang);
        case '<': return make_token(match('=') ? TokenType::LessEqual : TokenType::Less);
        case '>': return make_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
        case '&': return make_token(match('&') ? TokenType::AmpAmp : TokenType::Amp);
        case '|': return make_token(match('|') ? TokenType::PipePipe : TokenType::Pipe);

        // String literals
        case '"': return string();
    }

    return error_token("Unexpected character.");
}

/**
 * @brief Tokenizes the entire source code at once.
 * @return A vector of all tokens found in the source.
 */
std::vector<Token> Lexer::all_tokens() {
    std::vector<Token> tokens;
    Token token;
    do {
        token = next_token();
        tokens.push_back(token);
    } while (token.type != TokenType::Eof);
    return tokens;
}

// Consumes the current character and returns it.
char Lexer::advance() {
    if (is_at_end()) return '\0';
    _current++;
    return *(_current - 1);
}

// Returns the current character without consuming it.
char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return *_current;
}

// Returns the character after the current one without consuming it.
char Lexer::peek_next() const {
    if (_current + 1 >= _source.end()) return '\0';
    return *(_current + 1);
}

// Checks if the lexer has reached the end of the source code.
bool Lexer::is_at_end() const {
    return _current == _source.end();
}

// Creates a token of the given type.
Token Lexer::make_token(TokenType type) {
    std::string_view lexeme(_token_start, _current - _token_start);
    int column = static_cast<int>(_token_start - _line_start) + 1;
    return Token{type, lexeme, _line, column};
}

// Creates an error token with a specific message.
Token Lexer::error_token(const char* message) {
    int column = static_cast<int>(_token_start - _line_start) + 1;
    return Token{TokenType::Illegal, message, _line, column};
}

// Consumes and tokenizes a string literal.
Token Lexer::string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            _line++;
            _line_start = _current + 1;
        }
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    // Consume the closing quote.
    advance();
    return make_token(TokenType::String);
}

// Consumes and tokenizes a number (integer, float, or base-81).
Token Lexer::number() {
    while (is_digit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && is_digit(peek_next())) {
        advance(); // Consume the '.'
        while (is_digit(peek())) advance();
        return make_token(TokenType::Float);
    }

    // Look for a base-81 suffix.
    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) && *(_current + 2) == '1') {
        advance();
        advance();
        advance();
        return make_token(TokenType::Base81Integer);
    }

    return make_token(TokenType::Integer);
}

// Consumes and tokenizes an identifier or a keyword.
Token Lexer::identifier() {
    while (is_alpha(peek()) || is_digit(peek())) advance();

    std::string_view text(_token_start, _current - _token_start);

    // Check if the identifier is a reserved keyword.
    auto it = KEYWORDS.find(text);
    if (it != KEYWORDS.end()) {
        return make_token(it->second);
    }

    return make_token(TokenType::Identifier);
}

// Skips over whitespace and comments.
void Lexer::skip_whitespace_and_comments() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                _line++;
                advance();
                _line_start = _current;
                break;
            case '/':
                if (peek_next() == '/') {
                    // Single-line comment: consume until the end of the line.
                    while (peek() != '\n' && !is_at_end()) advance();
                } else if (peek_next() == '*') {
                    // Multi-line comment: consume until '*/'.
                    advance(); // Consume '/'
                    advance(); // Consume '*'
                    while (!(peek() == '*' && peek_next() == '/') && !is_at_end()) {
                        if (advance() == '\n') {
                            _line++;
                            _line_start = _current;
                        }
                    }
                    if (!is_at_end()) advance(); // Consume '*'
                    if (!is_at_end()) advance(); // Consume '/'
                } else {
                    // Not a comment, just a slash.
                    return;
                }
                break;
            default:
                return;
        }
    }
}

// Checks if the current character matches the expected one and consumes it if so.
bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (*_current != expected) return false;
    _current++;
    return true;
}

} // namespace frontend
} // namespace t81
