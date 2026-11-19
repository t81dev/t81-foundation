#include "t81/frontend/lexer.hpp"
#include <array>
#include <cctype>
#include <cstdint>

namespace t81 {
namespace frontend {

// MSVC FIX - These overloads MUST be in namespace t81::frontend, NOT anonymous
inline std::string_view make_sv(const char* b, const char* e) noexcept {
  return std::string_view(b, static_cast<std::size_t>(e - b));
}

template <class It>
inline std::string_view make_sv(It b, It e) noexcept {
  return std::string_view(&*b, static_cast<std::size_t>(std::distance(b, e)));
}

namespace {
constexpr uint8_t kCharAlpha = 1u << 0;
constexpr uint8_t kCharDigit = 1u << 1;
constexpr uint8_t kCharIdent = kCharAlpha | kCharDigit;
constexpr uint8_t kCharSpace = 1u << 2;

constexpr std::array<uint8_t, 256> make_char_table() noexcept {
  std::array<uint8_t, 256> t{};
  for (int c = 'a'; c <= 'z'; ++c) t[c] |= kCharAlpha;
  for (int c = 'A'; c <= 'Z'; ++c) t[c] |= kCharAlpha;
  t[static_cast<unsigned char>('_')] |= kCharAlpha;
  for (int c = '0'; c <= '9'; ++c) t[c] |= kCharDigit;
  t[static_cast<unsigned char>(' ')] |= kCharSpace;
  t[static_cast<unsigned char>('\r')] |= kCharSpace;
  t[static_cast<unsigned char>('\t')] |= kCharSpace;
  return t;
}

constexpr auto kCharClass = make_char_table();

inline bool is_alpha(char c) noexcept {
  return (kCharClass[static_cast<unsigned char>(c)] & kCharAlpha) != 0;
}

inline bool is_digit(char c) noexcept {
  return (kCharClass[static_cast<unsigned char>(c)] & kCharDigit) != 0;
}

inline bool is_ident_char(char c) noexcept {
  return (kCharClass[static_cast<unsigned char>(c)] & kCharIdent) != 0;
}

inline TokenType lookup_keyword(std::string_view text) noexcept {
  switch (text.size()) {
    case 2:
      if (text == "fn") return TokenType::Fn;
      if (text == "if") return TokenType::If;
      if (text == "in") return TokenType::In;
      if (text == "as") return TokenType::As;
      if (text == "i8") return TokenType::I8;
      if (text == "i2") return TokenType::I2;
      return TokenType::Identifier;
    case 3:
      if (text == "let") return TokenType::Let;
      if (text == "var") return TokenType::Var;
      if (text == "for") return TokenType::For;
      if (text == "mut") return TokenType::Mut;
      if (text == "i32") return TokenType::I32;
      if (text == "i16") return TokenType::I16;
      if (text == "map") return TokenType::Map;
      if (text == "set") return TokenType::Set;
      return TokenType::Identifier;
    case 4:
      if (text == "type") return TokenType::Type;
      if (text == "else") return TokenType::Else;
      if (text == "loop") return TokenType::Loop;
      if (text == "enum") return TokenType::Enum;
      if (text == "true") return TokenType::True;
      if (text == "void") return TokenType::Void;
      if (text == "bool") return TokenType::Bool;
      if (text == "Cell") return TokenType::Cell;
      if (text == "list") return TokenType::List;
      if (text == "tree") return TokenType::Tree;
      return TokenType::Identifier;
    case 5:
      if (text == "const") return TokenType::Const;
      if (text == "while") return TokenType::While;
      if (text == "infer") return TokenType::Infer;
      if (text == "train") return TokenType::Train;
      if (text == "agent") return TokenType::Agent;
      if (text == "break") return TokenType::Break;
      if (text == "match") return TokenType::Match;
      if (text == "false") return TokenType::False;
      if (text == "graph") return TokenType::Graph;
      return TokenType::Identifier;
    case 6:
      if (text == "return") return TokenType::Return;
      if (text == "module") return TokenType::Module;
      if (text == "export") return TokenType::Export;
      if (text == "record") return TokenType::Record;
      if (text == "assert") return TokenType::Assert;
      if (text == "matrix") return TokenType::Matrix;
      if (text == "tensor") return TokenType::Tensor;
      return TokenType::Identifier;
    case 7:
      if (text == "reflect") return TokenType::Reflect;
      if (text == "recurse") return TokenType::Recurse;
      if (text == "foreign") return TokenType::Foreign;
      if (text == "T81Prob") return TokenType::T81Prob;
      if (text == "T81Uint") return TokenType::T81Uint;
      return TokenType::Identifier;
    case 8:
      if (text == "infinite") return TokenType::Infinite;
      if (text == "behavior") return TokenType::Behavior;
      if (text == "continue") return TokenType::Continue;
      if (text == "T81Float") return TokenType::T81Float;
      if (text == "T81Fixed") return TokenType::T81Fixed;
      return TokenType::Identifier;
    case 9:
      if (text == "T81BigInt") return TokenType::T81BigInt;
      if (text == "T81Qutrit") return TokenType::T81Qutrit;
      if (text == "T81String") return TokenType::String;
      if (text == "T81Vector") return TokenType::T81Vector;
      return TokenType::Identifier;
    case 10:
      if (text == "T81Complex") return TokenType::T81Complex;
      return TokenType::Identifier;
    case 11:
      if (text == "distributed") return TokenType::Distributed;
      if (text == "T81Fraction") return TokenType::T81Fraction;
      return TokenType::Identifier;
    case 13:
      if (text == "T81Quaternion") return TokenType::T81Quaternion;
      return TokenType::Identifier;
    default:
      return TokenType::Identifier;
  }
}
}  // anonymous namespace

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

  // Handle multi-byte characters (specifically ∞)
  if (static_cast<unsigned char>(c) == 0xE2) {
    if (peek() == '\x88' && peek_next() == '\x9E') {
      advance();  // Consume 0x88
      advance();  // Consume 0x9E
      return infinite_literal();
    }
  }

  // Handle byte-string literals before identifier check
  if (c == 'b' && match('"')) return byte_string();

  if (is_alpha(c)) return identifier();
  if (is_digit(c)) return number();

  switch (c) {
    case ':':
      return symbol();
    case '(':
      return make_token(TokenType::LParen);
    case ')':
      return make_token(TokenType::RParen);
    case '{':
      return make_token(TokenType::LBrace);
    case '}':
      return make_token(TokenType::RBrace);
    case '[':
      return make_token(TokenType::LBracket);
    case ']':
      return make_token(TokenType::RBracket);
    case ',':
      return make_token(TokenType::Comma);
    case ';':
      return make_token(TokenType::Semicolon);
    case '@':
      return make_token(TokenType::At);
    case '?':
      return make_token(TokenType::Question);
    case '+':
      return make_token(TokenType::Plus);
    case '*':
      return make_token(match('*') ? TokenType::StarStar : TokenType::Star);
    case '%':
      return make_token(TokenType::Percent);
    case '^':
      return make_token(TokenType::Caret);
    case '~':
      return make_token(TokenType::Tilde);
    case '/':
      return make_token(TokenType::Slash);
    case '-':
      return make_token(match('>') ? TokenType::Arrow : TokenType::Minus);
    case '.':
      if (match('.')) {
        return make_token(match('=') ? TokenType::DotDotEq : TokenType::DotDot);
      }
      return make_token(TokenType::Dot);
    case '=':
      if (match('>')) return make_token(TokenType::FatArrow);
      return make_token(match('=') ? TokenType::EqualEqual : TokenType::Equal);
    case '!':
      return make_token(match('=') ? TokenType::BangEqual : TokenType::Bang);
    case '<':
      if (match('<')) return make_token(TokenType::LessLess);
      return make_token(match('=') ? TokenType::LessEqual : TokenType::Less);
    case '>':
      if (match('>')) {
        if (match('>')) return make_token(TokenType::GreaterGreaterGreater);
        return make_token(TokenType::GreaterGreater);
      }
      return make_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
    case '&':
      return make_token(match('&') ? TokenType::AmpAmp : TokenType::Amp);
    case '|':
      return make_token(match('|') ? TokenType::PipePipe : TokenType::Pipe);
    case '"':
      return string();
  }
  return error_token("Unexpected character.");
}

std::vector<Token> Lexer::all_tokens() {
  std::vector<Token> tokens;
  tokens.reserve(_source.size() / 5);
  Token token;
  do {
    token = next_token();
    tokens.push_back(token);
  } while (token.type != TokenType::Eof);
  return tokens;
}

char Lexer::advance() {
  if (is_at_end()) return '\0';
  return *(_current++);
}

char Lexer::peek() const { return is_at_end() ? '\0' : *_current; }

char Lexer::peek_next() const { return (_current + 1 >= _source.end()) ? '\0' : *(_current + 1); }

bool Lexer::is_at_end() const { return _current == _source.end(); }

Token Lexer::make_token(TokenType type) {
  std::string_view lexeme = make_sv(_token_start, _current);
  int column = static_cast<int>(_token_start - _line_start) + 1;
  return Token{type, lexeme, _line, column};
}

Token Lexer::error_token(const char* message) {
  int column = static_cast<int>(_token_start - _line_start) + 1;
  return Token{TokenType::Illegal, message, _line, column};
}

Token Lexer::string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n') {
      _line++;
      _line_start = _current + 1;
    }
    advance();
  }

  if (is_at_end()) return error_token("Unterminated string.");

  advance();  // closing quote
  return make_token(TokenType::String);
}

Token Lexer::byte_string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n') {
      _line++;
      _line_start = _current;
    }
    advance();
  }

  if (is_at_end()) return error_token("Unterminated byte string.");

  advance();  // closing quote
  return make_token(TokenType::ByteString);
}

Token Lexer::number() {
  // Check for hex literal: 0x... or 0X...
  if (*_token_start == '0' && (peek() == 'x' || peek() == 'X')) {
    advance();  // consume 'x'/'X'
    while (std::isxdigit(static_cast<unsigned char>(peek())) || peek() == '_') {
      advance();  // consume hex digit or separator
    }
    // Check for BigInt Base81 t81 suffix
    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) &&
        *(_current + 2) == '1') {
      advance();
      advance();
      advance();
      return make_token(TokenType::Base81Integer);
    }
    return make_token(TokenType::Integer);
  }

  // Check for binary literal: 0b... or 0B...
  if (*_token_start == '0' && (peek() == 'b' || peek() == 'B')) {
    advance();
    while (peek() == '0' || peek() == '1' || peek() == '_') {
      advance();
    }
    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) &&
        *(_current + 2) == '1') {
      advance();
      advance();
      advance();
      return make_token(TokenType::Base81Integer);
    }
    return make_token(TokenType::Integer);
  }

  // Check for octal literal: 0o... or 0O...
  if (*_token_start == '0' && (peek() == 'o' || peek() == 'O')) {
    advance();
    while ((peek() >= '0' && peek() <= '7') || peek() == '_') {
      advance();
    }
    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) &&
        *(_current + 2) == '1') {
      advance();
      advance();
      advance();
      return make_token(TokenType::Base81Integer);
    }
    return make_token(TokenType::Integer);
  }

  // Scan decimal digits, skipping '_' separators
  while (is_digit(peek()) || peek() == '_') advance();

  bool is_float_literal = false;
  if (peek() == '.' && is_digit(peek_next())) {
    is_float_literal = true;
    advance();
    while (is_digit(peek()) || peek() == '_') advance();
  }

  if (peek() == 'e' || peek() == 'E') {
    is_float_literal = true;
    char next = peek_next();
    if (next == '+' || next == '-') {
      advance();  // Consume 'e'
      advance();  // Consume '+' or '-'
      while (is_digit(peek())) advance();
    } else if (is_digit(next)) {
      advance();  // Consume 'e'
      while (is_digit(peek())) advance();
    }
  }

  // Float 'f' suffix: 0.0f -> Float
  if (is_float_literal && peek() == 'f') {
    advance();
    return make_token(TokenType::Float);
  }

  // Base81 suffix: t81
  if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) &&
      *(_current + 2) == '1') {
    advance();
    advance();
    advance();
    return make_token(is_float_literal ? TokenType::Base81Float : TokenType::Base81Integer);
  }

  // Trit 't' suffix: 1t -> Ternary (distinct from 't81' Base81 suffix)
  if (peek() == 't' && peek_next() != '8') {
    advance();
    return make_token(TokenType::Ternary);
  }

  return make_token(is_float_literal ? TokenType::Float : TokenType::Integer);
}

Token Lexer::identifier() {
  while (_current < _source.end() && is_ident_char(*_current)) {
    ++_current;
  }

  std::string_view text = make_sv(_token_start, _current);
  return make_token(lookup_keyword(text));
}

Token Lexer::symbol() {
  if (is_alpha(peek())) {
    while (is_alpha(peek()) || is_digit(peek())) advance();
    return make_token(TokenType::Symbol);
  }
  return make_token(TokenType::Colon);
}

Token Lexer::infinite_literal() { return make_token(TokenType::InfiniteLiteral); }

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
            if (advance() == '\n') {
              _line++;
              _line_start = _current;
            }
          }
          if (!is_at_end()) advance();
          if (!is_at_end()) advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

Token Lexer::peek_next_token() {
  auto saved_current = _current;
  auto saved_line = _line;
  auto saved_line_start = _line_start;
  auto saved_token_start = _token_start;
  Token token = next_token();
  _current = saved_current;
  _line = saved_line;
  _line_start = saved_line_start;
  _token_start = saved_token_start;
  return token;
}

bool Lexer::match(char expected) {
  if (is_at_end() || *_current != expected) return false;
  _current++;
  return true;
}

}  // namespace frontend
}  // namespace t81
