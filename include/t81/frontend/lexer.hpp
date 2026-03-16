#ifndef T81_FRONTEND_LEXER_HPP
#define T81_FRONTEND_LEXER_HPP

#include <string>
#include <string_view>
#include <vector>

namespace t81 {
namespace frontend {

enum class TokenType {
  // Keywords
  Module,
  Type,
  Const,
  Export,
  Fn,
  Let,
  Var,
  Record,
  Enum,
  If,
  Else,
  For,
  In,
  While,
  Loop,
  Reflect,
  Recurse,
  Distributed,
  Infinite,
  Infer,
  Train,
  Agent,
  Behavior,
  Foreign,
  Break,
  Continue,
  Return,
  Assert,
  As,
  Mut,
  Match,
  True,
  False,

  // Type Keywords
  Void,
  Bool,
  I32,
  I16,
  I8,
  I2,
  T81BigInt,
  T81Float,
  T81Fraction,
  T81Fixed,
  T81Complex,
  T81Quaternion,
  Cell,
  T81Qutrit,
  T81Uint,
  T81String,
  T81Vector,
  Matrix,
  Tensor,
  Graph,
  // Added missing collection types
  List,
  Map,
  Set,
  Tree,

  // Literals
  Integer,
  Float,
  T81Prob,
  String,
  ByteString,
  Ternary,
  Base81Integer,
  Base81Float,
  Symbol,
  InfiniteLiteral,

  // Identifier
  Identifier,

  // Operators
  Plus,
  Minus,
  Star,
  StarStar,
  Slash,
  Percent,
  Equal,
  EqualEqual,
  Bang,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Amp,
  AmpAmp,
  Pipe,
  PipePipe,
  Caret,
  Tilde,
  Question,
  LessLess,
  GreaterGreater,
  GreaterGreaterGreater,

  // Punctuation
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Colon,
  Semicolon,
  Arrow,
  FatArrow,
  DotDot,
  DotDotEq,
  Dot,

  // Special
  At,

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
  Token peek_next_token();

private:
  char advance();
  char peek() const;
  char peek_next() const;
  bool is_at_end() const;

  Token make_token(TokenType type);
  Token error_token(const char* message);
  Token string();
  Token byte_string();
  Token number();
  Token identifier();
  Token symbol();
  Token infinite_literal();

  void skip_whitespace_and_comments();
  bool match(char expected);

  std::string_view _source;
  std::string_view::iterator _current;
  std::string_view::iterator _line_start;
  std::string_view::iterator _token_start;
  int _line;
};

}  // namespace frontend
}  // namespace t81

#endif  // T81_FRONTEND_LEXER_HPP
