#include "t81/crypto/sha3.hpp"
#include "t81/frontend/ast.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/ir.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "test_runtime_check.hpp"

namespace fs = std::filesystem;
using namespace t81;

namespace {

fs::path fixture_root() {
  return fs::path(__FILE__).parent_path().parent_path() / "fixtures" / "t81lang_determinism";
}

std::string read_text(const fs::path& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string trim_ascii(std::string text) {
  const auto is_space = [](unsigned char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
  };
  while (!text.empty() && is_space(static_cast<unsigned char>(text.front()))) {
    text.erase(text.begin());
  }
  while (!text.empty() && is_space(static_cast<unsigned char>(text.back()))) {
    text.pop_back();
  }
  return text;
}

std::vector<uint8_t> as_u8(std::string_view text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

std::string token_repr(const frontend::Token& token) {
  std::ostringstream os;
  os << "{t=" << static_cast<int>(token.type) << ",lex=\"" << token.lexeme << "\"}";
  return os.str();
}

std::string pattern_repr(const frontend::MatchPattern& pattern);
std::string expr_repr(const frontend::Expr* expr);
std::string stmt_repr(const frontend::Stmt* stmt);

std::string pattern_repr(const frontend::MatchPattern& pattern) {
  using Kind = frontend::MatchPattern::Kind;
  std::ostringstream os;
  os << "(pattern ";
  switch (pattern.kind) {
    case Kind::None:
      os << "none";
      break;
    case Kind::Identifier:
      os << "id " << token_repr(pattern.identifier);
      if (pattern.binding_is_wildcard) {
        os << " wildcard";
      }
      break;
    case Kind::Tuple:
      os << "tuple";
      for (const auto& binding : pattern.tuple_bindings) {
        os << " " << token_repr(binding);
      }
      break;
    case Kind::Record:
      os << "record";
      for (const auto& [field, binding] : pattern.record_bindings) {
        os << " (" << token_repr(field) << " " << token_repr(binding) << ")";
      }
      break;
    case Kind::Variant:
      os << "variant " << token_repr(pattern.variant_name);
      if (pattern.variant_payload) {
        os << " payload=" << pattern_repr(*pattern.variant_payload);
      }
      break;
  }
  os << ")";
  return os.str();
}

std::string expr_repr(const frontend::Expr* expr) {
  if (!expr) {
    return "null-expr";
  }

  if (const auto* node = dynamic_cast<const frontend::BinaryExpr*>(expr)) {
    return "(binary " + token_repr(node->op) + " " + expr_repr(node->left.get()) + " " +
           expr_repr(node->right.get()) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::UnaryExpr*>(expr)) {
    return "(unary " + token_repr(node->op) + " " + expr_repr(node->right.get()) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::LiteralExpr*>(expr)) {
    return "(literal " + token_repr(node->value) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::GroupingExpr*>(expr)) {
    return "(group " + expr_repr(node->expression.get()) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::VariableExpr*>(expr)) {
    return "(var " + token_repr(node->name) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::CallExpr*>(expr)) {
    std::ostringstream os;
    os << "(call " << expr_repr(node->callee.get()) << " args";
    for (const auto& arg : node->arguments) {
      os << " " << expr_repr(arg.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::AssignExpr*>(expr)) {
    return "(assign " + expr_repr(node->target.get()) + " " + expr_repr(node->value.get()) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::MatchExpr*>(expr)) {
    std::ostringstream os;
    os << "(match " << expr_repr(node->scrutinee.get());
    for (const auto& arm : node->arms) {
      os << " (arm " << token_repr(arm.keyword) << " " << pattern_repr(arm.pattern);
      if (arm.guard) {
        os << " guard=" << expr_repr(arm.guard.get());
      }
      os << " -> " << expr_repr(arm.expression.get()) << ")";
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::VectorLiteralExpr*>(expr)) {
    std::ostringstream os;
    os << "(vector " << token_repr(node->token);
    for (const auto& element : node->elements) {
      os << " " << expr_repr(element.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::FieldAccessExpr*>(expr)) {
    return "(field " + expr_repr(node->object.get()) + " " + token_repr(node->field) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::RecordLiteralExpr*>(expr)) {
    std::ostringstream os;
    os << "(record-lit " << token_repr(node->type_name);
    for (const auto& [field, value] : node->fields) {
      os << " (" << token_repr(field) << " " << expr_repr(value.get()) << ")";
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::EnumLiteralExpr*>(expr)) {
    std::ostringstream os;
    os << "(enum-lit " << token_repr(node->enum_name) << " " << token_repr(node->variant);
    if (node->payload) {
      os << " " << expr_repr(node->payload.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::SimpleTypeExpr*>(expr)) {
    return "(type-simple " + token_repr(node->name) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::GenericTypeExpr*>(expr)) {
    std::ostringstream os;
    os << "(type-generic " << token_repr(node->name);
    for (size_t i = 0; i < node->param_count; ++i) {
      os << " " << expr_repr(node->params[i].get());
    }
    os << ")";
    return os.str();
  }

  return "unknown-expr";
}

std::string stmt_repr(const frontend::Stmt* stmt) {
  if (!stmt) {
    return "null-stmt";
  }

  if (const auto* node = dynamic_cast<const frontend::ExpressionStmt*>(stmt)) {
    return "(expr-stmt " + expr_repr(node->expression.get()) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::VarStmt*>(stmt)) {
    std::ostringstream os;
    os << "(var " << token_repr(node->name);
    if (node->type) {
      os << " type=" << expr_repr(node->type.get());
    }
    if (node->initializer) {
      os << " init=" << expr_repr(node->initializer.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::LetStmt*>(stmt)) {
    std::ostringstream os;
    os << "(let " << token_repr(node->name);
    if (node->type) {
      os << " type=" << expr_repr(node->type.get());
    }
    if (node->initializer) {
      os << " init=" << expr_repr(node->initializer.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::BlockStmt*>(stmt)) {
    std::ostringstream os;
    os << "(block";
    for (const auto& sub : node->statements) {
      os << " " << stmt_repr(sub.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::IfStmt*>(stmt)) {
    std::ostringstream os;
    os << "(if " << expr_repr(node->condition.get())
       << " then=" << stmt_repr(node->then_branch.get());
    if (node->else_branch) {
      os << " else=" << stmt_repr(node->else_branch.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::WhileStmt*>(stmt)) {
    return "(while " + expr_repr(node->condition.get()) + " " + stmt_repr(node->body.get()) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::LoopStmt*>(stmt)) {
    std::ostringstream os;
    os << "(loop bound-kind=" << static_cast<int>(node->bound_kind);
    if (node->bound_value) {
      os << " bound=" << *node->bound_value;
    }
    if (node->guard_expression) {
      os << " guard=" << expr_repr(node->guard_expression.get());
    }
    os << " body";
    for (const auto& sub : node->body) {
      os << " " << stmt_repr(sub.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::ReturnStmt*>(stmt)) {
    std::ostringstream os;
    os << "(return " << token_repr(node->keyword);
    if (node->value) {
      os << " " << expr_repr(node->value.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::BreakStmt*>(stmt)) {
    return "(break " + token_repr(node->keyword) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::ContinueStmt*>(stmt)) {
    return "(continue " + token_repr(node->keyword) + ")";
  }
  if (const auto* node = dynamic_cast<const frontend::FunctionStmt*>(stmt)) {
    std::ostringstream os;
    os << "(fn " << token_repr(node->name);
    if (!node->generic_params.empty()) {
      os << " generics";
      for (const auto& gp : node->generic_params) {
        os << " " << token_repr(gp);
      }
    }
    os << " params";
    for (const auto& param : node->params) {
      os << " (" << token_repr(param.name);
      if (param.type) {
        os << " " << expr_repr(param.type.get());
      }
      os << ")";
    }
    if (node->return_type) {
      os << " ret=" << expr_repr(node->return_type.get());
    }
    os << " body";
    for (const auto& sub : node->body) {
      os << " " << stmt_repr(sub.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::TypeDecl*>(stmt)) {
    std::ostringstream os;
    os << "(type " << token_repr(node->name) << " params";
    for (const auto& param : node->params) {
      os << " " << token_repr(param);
    }
    if (node->alias) {
      os << " alias=" << expr_repr(node->alias.get());
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::RecordDecl*>(stmt)) {
    std::ostringstream os;
    os << "(record " << token_repr(node->name);
    if (node->schema_version) {
      os << " schema=" << *node->schema_version;
    }
    if (node->module_path) {
      os << " module=" << *node->module_path;
    }
    for (const auto& field : node->fields) {
      os << " (field " << token_repr(field.name);
      if (field.type) {
        os << " " << expr_repr(field.type.get());
      }
      os << ")";
    }
    os << ")";
    return os.str();
  }
  if (const auto* node = dynamic_cast<const frontend::EnumDecl*>(stmt)) {
    std::ostringstream os;
    os << "(enum " << token_repr(node->name);
    if (node->schema_version) {
      os << " schema=" << *node->schema_version;
    }
    if (node->module_path) {
      os << " module=" << *node->module_path;
    }
    for (const auto& variant : node->variants) {
      os << " (variant " << token_repr(variant.name);
      if (variant.payload) {
        os << " " << expr_repr(variant.payload.get());
      }
      os << ")";
    }
    os << ")";
    return os.str();
  }

  return "unknown-stmt";
}

std::string ast_repr(const std::vector<std::unique_ptr<frontend::Stmt>>& stmts) {
  std::ostringstream os;
  os << "(program";
  for (const auto& stmt : stmts) {
    os << " " << stmt_repr(stmt.get());
  }
  os << ")";
  return os.str();
}

const char* ir_opcode_name(tisc::ir::Opcode op) {
  using O = tisc::ir::Opcode;
  switch (op) {
    case t81::tisc::ir::Opcode::F2FRAC:
      return "F2FRAC";
    case t81::tisc::ir::Opcode::FRAC2F:
      return "FRAC2F";
    case t81::tisc::ir::Opcode::TTENDOT:
      return "TTENDOT";
    case t81::tisc::ir::Opcode::BITAND:
      return "BITAND";
    case t81::tisc::ir::Opcode::BITOR:
      return "BITOR";
    case t81::tisc::ir::Opcode::BITXOR:
      return "BITXOR";
    case t81::tisc::ir::Opcode::BITNOT:
      return "BITNOT";
    case t81::tisc::ir::Opcode::BITSHL:
      return "BITSHL";
    case t81::tisc::ir::Opcode::BITSHR:
      return "BITSHR";
    case t81::tisc::ir::Opcode::BITUSHR:
      return "BITUSHR";
    case t81::tisc::ir::Opcode::SYMLOAD:
      return "SYMLOAD";
    case t81::tisc::ir::Opcode::SYMREWRITE:
      return "SYMREWRITE";
    case t81::tisc::ir::Opcode::SYMCANON:
      return "SYMCANON";
    case t81::tisc::ir::Opcode::SYMCONFLUENCE:
      return "SYMCONFLUENCE";
    case t81::tisc::ir::Opcode::MapNew:
      return "MapNew";
    case t81::tisc::ir::Opcode::MapPut:
      return "MapPut";
    case t81::tisc::ir::Opcode::MapGet:
      return "MapGet";
    case t81::tisc::ir::Opcode::MapHas:
      return "MapHas";
    case t81::tisc::ir::Opcode::MapRemove:
      return "MapRemove";
    case t81::tisc::ir::Opcode::MapKeys:
      return "MapKeys";
    case t81::tisc::ir::Opcode::MapSize:
      return "MapSize";
    case t81::tisc::ir::Opcode::SetNew:
      return "SetNew";
    case t81::tisc::ir::Opcode::SetAdd:
      return "SetAdd";
    case t81::tisc::ir::Opcode::SetRemove:
      return "SetRemove";
    case t81::tisc::ir::Opcode::SetHas:
      return "SetHas";
    case t81::tisc::ir::Opcode::SetSize:
      return "SetSize";

    case O::ADD:
      return "ADD";
    case O::SUB:
      return "SUB";
    case O::MUL:
      return "MUL";
    case O::DIV:
      return "DIV";
    case O::MOD:
      return "MOD";
    case O::NEG:
      return "NEG";
    case O::FADD:
      return "FADD";
    case O::FSUB:
      return "FSUB";
    case O::FMUL:
      return "FMUL";
    case O::FDIV:
      return "FDIV";
    case O::FSIN:
      return "FSIN";
    case O::FCOS:
      return "FCOS";
    case O::FTAN:
      return "FTAN";
    case O::FASIN:
      return "FASIN";
    case O::FACOS:
      return "FACOS";
    case O::FATAN:
      return "FATAN";
    case O::FSINH:
      return "FSINH";
    case O::FCOSH:
      return "FCOSH";
    case O::FTANH:
      return "FTANH";
    case O::FSQRT:
      return "FSQRT";
    case O::FEXP:
      return "FEXP";
    case O::FLOG:
      return "FLOG";
    case O::FPOW:
      return "FPOW";
    case O::FRACADD:
      return "FRACADD";
    case O::FRACSUB:
      return "FRACSUB";
    case O::FRACMUL:
      return "FRACMUL";
    case O::FRACDIV:
      return "FRACDIV";
    case O::CMP:
      return "CMP";
    case O::MOV:
      return "MOV";
    case O::LOADI:
      return "LOADI";
    case O::LOAD:
      return "LOAD";
    case O::STORE:
      return "STORE";
    case O::PUSH:
      return "PUSH";
    case O::POP:
      return "POP";
    case O::JMP:
      return "JMP";
    case O::JZ:
      return "JZ";
    case O::JNZ:
      return "JNZ";
    case O::JN:
      return "JN";
    case O::JP:
      return "JP";
    case O::CALL:
      return "CALL";
    case O::RET:
      return "RET";
    case O::I2F:
      return "I2F";
    case O::F2I:
      return "F2I";
    case O::I2FRAC:
      return "I2FRAC";
    case O::FRAC2I:
      return "FRAC2I";
    case O::MAKE_OPTION_SOME:
      return "MAKE_OPTION_SOME";
    case O::MAKE_OPTION_NONE:
      return "MAKE_OPTION_NONE";
    case O::MAKE_RESULT_OK:
      return "MAKE_RESULT_OK";
    case O::MAKE_RESULT_ERR:
      return "MAKE_RESULT_ERR";
    case O::OPTION_IS_SOME:
      return "OPTION_IS_SOME";
    case O::OPTION_UNWRAP:
      return "OPTION_UNWRAP";
    case O::RESULT_IS_OK:
      return "RESULT_IS_OK";
    case O::RESULT_UNWRAP_OK:
      return "RESULT_UNWRAP_OK";
    case O::RESULT_UNWRAP_ERR:
      return "RESULT_UNWRAP_ERR";
    case O::MAKE_ENUM_VARIANT:
      return "MAKE_ENUM_VARIANT";
    case O::MAKE_ENUM_VARIANT_PAYLOAD:
      return "MAKE_ENUM_VARIANT_PAYLOAD";
    case O::ENUM_IS_VARIANT:
      return "ENUM_IS_VARIANT";
    case O::ENUM_UNWRAP_PAYLOAD:
      return "ENUM_UNWRAP_PAYLOAD";
    case O::MAKE_COMPLEX:
      return "MAKE_COMPLEX";
    case O::NOP:
      return "NOP";
    case O::HALT:
      return "HALT";
    case O::TRAP:
      return "TRAP";
    case O::PRINT:
      return "PRINT";
    case O::STRLEN:
      return "STRLEN";
    case O::STREMPTY:
      return "STREMPTY";
    case O::VECLEN:
      return "VECLEN";
    case O::VECEMPTY:
      return "VECEMPTY";
    case O::VECFIRST:
      return "VECFIRST";
    case O::VECLAST:
      return "VECLAST";
    case O::VECPUSH:
      return "VECPUSH";
    case O::VECPOP:
      return "VECPOP";
    case O::STRCONCAT:
      return "STRCONCAT";
    case O::STRSTARTSWITH:
      return "STRSTARTSWITH";
    case O::STRENDSWITH:
      return "STRENDSWITH";
    case O::STRCONTAINS:
      return "STRCONTAINS";
    case O::STRINDEXOF:
      return "STRINDEXOF";
    case O::STRREPLACE:
      return "STRREPLACE";
    case O::STRVECNEW:
      return "STRVECNEW";
    case O::STRVECPUSH:
      return "STRVECPUSH";
    case O::STRSPLIT:
      return "STRSPLIT";
    case O::STRJOIN:
      return "STRJOIN";
    case O::WEIGHTS_LOAD:
      return "WEIGHTS_LOAD";
    case O::META_READ:
      return "META_READ";
    case O::META_WRITE:
      return "META_WRITE";
    case O::META_REFLECT:
      return "META_REFLECT";
    case O::META_REFINE:
      return "META_REFINE";
    case t81::tisc::ir::Opcode::TMATMUL:
      return "TMATMUL";
    case t81::tisc::ir::Opcode::TVECADD:
      return "TVECADD";
    case t81::tisc::ir::Opcode::TGET:
      return "TGET";
    case t81::tisc::ir::Opcode::TNEW:
      return "TNEW";
    case t81::tisc::ir::Opcode::TSET:
      return "TSET";
    case t81::tisc::ir::Opcode::TNEURAL_FWD:
      return "TNEURAL_FWD";
    case t81::tisc::ir::Opcode::TNEURAL_BWD:
      return "TNEURAL_BWD";
    case t81::tisc::ir::Opcode::GOSSIP:
      return "GOSSIP";
    case t81::tisc::ir::Opcode::MERGE:
      return "MERGE";
    case t81::tisc::ir::Opcode::TICKSYNC:
      return "TICKSYNC";
    case t81::tisc::ir::Opcode::COHERENCE:
      return "COHERENCE";
    case t81::tisc::ir::Opcode::DISTSEAL:
      return "DISTSEAL";
    case O::LABEL:
      return "LABEL";
  }
  return "UNKNOWN";
}

std::string ir_repr(const tisc::ir::IntermediateProgram& ir_program) {
  std::ostringstream os;
  os << "(ir";
  for (const auto& inst : ir_program.instructions()) {
    os << " (" << ir_opcode_name(inst.opcode) << " prim=" << static_cast<int>(inst.primitive)
       << " bool=" << (inst.boolean_result ? 1 : 0) << " conv=" << (inst.is_conversion ? 1 : 0)
       << " rel=" << static_cast<int>(inst.relation)
       << " litkind=" << static_cast<int>(inst.literal_kind);
    if (inst.text_literal) {
      os << " text=\"" << *inst.text_literal << "\"";
    }
    if (!inst.operands.empty()) {
      os << " ops";
      for (const auto& operand : inst.operands) {
        std::visit(
            [&os](auto&& value) {
              using T = std::decay_t<decltype(value)>;
              if constexpr (std::is_same_v<T, tisc::ir::Register>) {
                os << " R" << value.index;
              } else if constexpr (std::is_same_v<T, tisc::ir::Immediate>) {
                os << " I" << value.value;
              } else if constexpr (std::is_same_v<T, tisc::ir::Label>) {
                os << " L" << value.id;
              }
            },
            operand);
      }
    }
    os << ")";
  }
  os << ")";
  return os.str();
}

struct CanonPair {
  std::string ast;
  std::string ir;
};

CanonPair compile_to_canon(const std::string& source, const std::string& diag_name) {
  frontend::Lexer parser_lexer(source);
  frontend::Parser parser(parser_lexer, diag_name);
  auto stmts = parser.parse();
  if (parser.had_error()) {
    throw std::runtime_error("Parse failure for " + diag_name);
  }

  frontend::SemanticAnalyzer semantic(stmts, diag_name);
  semantic.analyze();
  if (semantic.had_error()) {
    throw std::runtime_error("Semantic failure for " + diag_name);
  }

  frontend::IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&semantic);
  auto ir_program = ir_gen.generate(stmts);

  CanonPair out;
  out.ast = ast_repr(stmts);
  out.ir = ir_repr(ir_program);
  return out;
}

void test_ast_ir_compile_repeat_hash_gate() {
  const fs::path root = fixture_root();
  if (!fs::exists(root)) {
    throw std::runtime_error("Missing fixture root: " + root.string());
  }

  std::vector<fs::path> fixtures;
  for (const auto& entry : fs::directory_iterator(root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".t81") {
      fixtures.push_back(entry.path());
    }
  }
  std::sort(fixtures.begin(), fixtures.end());
  T81_TEST_CHECK(!fixtures.empty());

  std::ostringstream aggregate;
  aggregate << "t81lang-ast-ir-canon-v1\n";

  for (const auto& fixture : fixtures) {
    const std::string source = read_text(fixture);
    const auto pass_a = compile_to_canon(source, fixture.string());
    const auto pass_b = compile_to_canon(source, fixture.string());
    T81_TEST_CHECK(pass_a.ast == pass_b.ast);
    T81_TEST_CHECK(pass_a.ir == pass_b.ir);

    const auto ast_hash = crypto::sha3_512_hex(as_u8(pass_a.ast));
    const auto ir_hash = crypto::sha3_512_hex(as_u8(pass_a.ir));
    aggregate << fixture.filename().string() << "\n";
    aggregate << "ast=" << ast_hash << "\n";
    aggregate << "ir=" << ir_hash << "\n";
  }

  const std::string aggregate_hash = crypto::sha3_512_hex(as_u8(aggregate.str()));
  const fs::path expected_path = root / "t81lang_ast_ir_repro_hash.txt";
  const std::string expected_hash = trim_ascii(read_text(expected_path));
  if (aggregate_hash != expected_hash) {
    std::ostringstream msg;
    msg << "unexpected AST/IR reproducibility hash drift: expected=" << expected_hash
        << " actual=" << aggregate_hash;
    throw std::runtime_error(msg.str());
  }
}

}  // namespace

int main() {
  test_ast_ir_compile_repeat_hash_gate();
  std::cout << "e2e ast/ir canonical determinism test passed!\n";
  return 0;
}
