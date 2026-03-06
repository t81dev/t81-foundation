#include "t81/frontend/lexer.hpp"
#include <cctype>
#include <unordered_map>

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
    {"loop", TokenType::Loop},
    {"reflect", TokenType::Reflect},
    {"recurse", TokenType::Recurse},
    {"distributed", TokenType::Distributed},
    {"infinite", TokenType::Infinite},
    {"infer", TokenType::Infer},
    {"train", TokenType::Train},
    {"record", TokenType::Record},
    {"enum", TokenType::Enum},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"return", TokenType::Return},
    {"assert", TokenType::Assert},
    {"as", TokenType::As},
    {"mut", TokenType::Mut},
    {"match", TokenType::Match},
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
    {"T81Fixed", TokenType::T81Fixed},
    {"T81Complex", TokenType::T81Complex},
    {"T81Quaternion", TokenType::T81Quaternion},
    {"T81Prob", TokenType::T81Prob},
    {"Cell", TokenType::Cell},
    {"T81Qutrit", TokenType::T81Qutrit},
    {"T81Uint", TokenType::T81Uint},
    {"T81String", TokenType::String},
    {"T81Vector", TokenType::T81Vector},
    {"matrix", TokenType::Matrix},
    {"tensor", TokenType::Tensor},
    {"graph", TokenType::Graph},
    {"list", TokenType::List},
    {"map", TokenType::Map},
    {"set", TokenType::Set},
    {"tree", TokenType::Tree},
};

bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

bool is_digit(char c) { return c >= '0' && c <= '9'; }
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
    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) && *(_current + 2) == '1') {
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
    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) && *(_current + 2) == '1') {
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
    if (peek() == 't' && peek_next() == '8' && (_current + 2 < _source.end()) && *(_current + 2) == '1') {
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
  while (true) {
    char next = peek();
    if (is_alpha(next) || is_digit(next)) {
      advance();
      continue;
    }
    break;
  }

  std::string_view text = make_sv(_token_start, _current);

  if (auto it = KEYWORDS.find(text); it != KEYWORDS.end()) {
    return make_token(it->second);
  }
  
  // Check for T81Fixed suffix: 1.25fx -> T81Fixed
  // Only treat as T81Fixed if it ends with 'fx' and contains a decimal point
  if (text.length() > 2 && text.substr(text.length() - 2) == "fx" && text.find('.') != std::string_view::npos) {
    return make_token(TokenType::T81Fixed);
  }
  
  return make_token(TokenType::Identifier);
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
