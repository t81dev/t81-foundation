#include "t81/frontend/lexer.hpp"

#include <unordered_map>

namespace t81 {
namespace frontend {

namespace {

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

bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

} // namespace

Lexer::Lexer(std::string_view source)
    : _source(source),
      _current(_source.begin()),
      _line_start(_source.begin()),
      _token_start(_source.begin()),
      _line(1) {}

Token Lexer::next_token() {
    skip_whitespace_and_comments();

    _token_start = _current;

    if (is_at_end()) return make_token(TokenType::Eof);

    char c = advance();

    if (is_alpha(c)) return identifier();
    if (is_digit(c)) return number();

    switch (c) {
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

        case '-': return make_token(match('>') ? TokenType::Arrow : TokenType::Minus);
        case '.': return make_token(match('.') ? TokenType::DotDot : TokenType::Illegal);
        case '=': return make_token(match('=') ? TokenType::EqualEqual : TokenType::Equal);
        case '!': return make_token(match('=') ? TokenType::BangEqual : TokenType::Bang);
        case '<': return make_token(match('=') ? TokenType::LessEqual : TokenType::Less);
        case '>': return make_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
        case '&': return make_token(match('&') ? TokenType::AmpAmp : TokenType::Amp);
        case '|': return make_token(match('|') ? TokenType::PipePipe : TokenType::Pipe);

        case '/': return make_token(TokenType::Slash);

        case '"': return string();
    }

    return error_token("Unexpected character.");
}

std::vector<Token> Lexer::all_tokens() {
    std::vector<Token> tokens;
    Token token;
    do {
        token = next_token();
        tokens.push_back(token);
    } while (token.type != TokenType::Eof);
    return tokens;
}

char Lexer::advance() {
    if (is_at_end()) return '\0';
    _current++;
    return *(_current - 1);
}

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return *_current;
}

char Lexer::peek_next() const {
    if (_current + 1 >= _source.end()) return '\0';
    return *(_current + 1);
}

bool Lexer::is_at_end() const {
    return _current == _source.end();
}

Token Lexer::make_token(TokenType type) {
    std::string_view lexeme(_token_start, _current - _token_start);
    int column = static_cast<int>(_token_start - _line_start) + 1;
    return Token{type, lexeme, _line, column};
}

Token Lexer::error_token(const char* message) {
    int column = static_cast<int>(_token_start - _line_start) + 1;
    return Token{TokenType::Illegal, message, _line, column};
}

Token Lexer::string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') _line++;
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    advance();
    return make_token(TokenType::String);
}

Token Lexer::number() {
    while (is_digit(peek())) advance();

    if (peek() == '.' && is_digit(peek_next())) {
        advance();
        while (is_digit(peek())) advance();
        return make_token(TokenType::Float);
    }

    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) && *(_current + 2) == '1') {
        advance();
        advance();
        advance();
        return make_token(TokenType::Base81Integer);
    }

    // Ternary literals are handled as a special case by the parser, as they can
    // be ambiguous with unary minus. For now, we will tokenize them as integers.
    return make_token(TokenType::Integer);
}

Token Lexer::identifier() {
    while (is_alpha(peek()) || is_digit(peek())) advance();

    std::string_view text(_token_start, _current - _token_start);

    auto it = KEYWORDS.find(text);
    if (it != KEYWORDS.end()) {
        return make_token(it->second);
    }

    return make_token(TokenType::Identifier);
}

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
                    while (peek() != '\n' && !is_at_end()) advance();
                } else if (peek_next() == '*') {
                    advance();
                    advance();
                    while (!(peek() == '*' && peek_next() == '/') && !is_at_end()) {
                     if(advance() == '\n') {
                        _line++;
                        _line_start = _current;
                     }
                    }
                    if(!is_at_end()) advance();
                    if(!is_at_end()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (*_current != expected) return false;
    _current++;
    return true;
}

} // namespace frontend
} // namespace t81
