#include "t81/fraction.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/program.hpp"
#include "t81/isa/type_alias.hpp"
#include "t81/tensor.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "tisc_binary_metadata_roundtrip_property_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

std::vector<std::uint8_t> read_bytes(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>());
}

std::string rand_ident(std::mt19937_64& rng, const std::string& prefix) {
  const auto n = static_cast<int>(rng() % 100000);
  return prefix + "_" + std::to_string(n);
}

t81::tisc::TypeAliasMetadata random_alias(std::mt19937_64& rng) {
  using t81::tisc::FieldInfo;
  using t81::tisc::StructuralKind;
  using t81::tisc::TypeAliasMetadata;
  using t81::tisc::VariantInfo;

  TypeAliasMetadata m;
  m.name = rand_ident(rng, "Alias");
  m.schema_version = static_cast<std::uint32_t>(1 + (rng() % 5));
  m.module_path = rand_ident(rng, "module");

  const auto kind_pick = static_cast<int>(rng() % 3);
  if (kind_pick == 0) {
    m.kind = StructuralKind::TypeAlias;
    m.params = {rand_ident(rng, "T"), rand_ident(rng, "U")};
    m.alias = "Vector[i81]";
  } else if (kind_pick == 1) {
    m.kind = StructuralKind::Record;
    m.fields = {
        FieldInfo{rand_ident(rng, "field"), "i81"},
        FieldInfo{rand_ident(rng, "flag"), "bool"},
    };
  } else {
    m.kind = StructuralKind::Enum;
    m.variants = {
        VariantInfo{rand_ident(rng, "A"), std::nullopt},
        VariantInfo{rand_ident(rng, "B"), std::optional<std::string>{"i81"}},
    };
  }
  return m;
}

t81::tisc::EnumMetadata random_enum_meta(std::mt19937_64& rng) {
  t81::tisc::EnumMetadata e;
  e.enum_id = static_cast<int>(rng() % 1000);
  e.name = rand_ident(rng, "E");
  e.variants.push_back({rand_ident(rng, "V0"), std::nullopt, 0});
  e.variants.push_back({rand_ident(rng, "V1"), std::optional<std::string>{"i81"}, 1});
  return e;
}

t81::tisc::Insn random_insn(std::mt19937_64& rng) {
  const auto raw_opcode =
      static_cast<std::uint8_t>(rng() % (static_cast<std::uint8_t>(t81::tisc::Opcode::Print) + 1));
  t81::tisc::Insn insn;
  insn.opcode = static_cast<t81::tisc::Opcode>(raw_opcode);
  insn.a = static_cast<std::int32_t>(rng() % 64);
  insn.b = static_cast<std::int64_t>(static_cast<std::int64_t>(rng() % 2000) - 1000);
  insn.c = static_cast<std::int32_t>(static_cast<std::int32_t>(rng() % 64) - 32);
  const auto raw_kind = static_cast<std::uint8_t>(
      rng() % (static_cast<std::uint8_t>(t81::tisc::LiteralKind::ShapeHandle) + 1));
  insn.literal_kind = static_cast<t81::tisc::LiteralKind>(raw_kind);
  return insn;
}

t81::tisc::Program random_program(std::mt19937_64& rng) {
  t81::tisc::Program p;
  const auto insn_count = static_cast<int>(1 + (rng() % 40));
  p.insns.reserve(static_cast<std::size_t>(insn_count));
  for (int i = 0; i < insn_count; ++i) {
    p.insns.push_back(random_insn(rng));
  }

  const auto float_count = static_cast<int>(rng() % 8);
  for (int i = 0; i < float_count; ++i) {
    p.float_pool.push_back(static_cast<double>(static_cast<int>(rng() % 200) - 100) / 10.0);
  }

  const auto frac_count = static_cast<int>(rng() % 6);
  for (int i = 0; i < frac_count; ++i) {
    const auto num = static_cast<std::int64_t>(static_cast<int>(rng() % 200) - 100);
    const auto den = static_cast<std::int64_t>(1 + (rng() % 19));
    p.fraction_pool.emplace_back(t81::T81BigInt::from_i64(num), t81::T81BigInt::from_i64(den));
  }

  const auto bigint_count = static_cast<int>(rng() % 6);
  for (int i = 0; i < bigint_count; ++i) {
    const auto hi =
        t81::T81BigInt::from_i64(static_cast<std::int64_t>(rng() % 9000000000000000000ULL));
    const auto lo =
        t81::T81BigInt::from_i64(static_cast<std::int64_t>(rng() % 9000000000000000000ULL));
    t81::T81BigInt v = (hi * t81::T81BigInt::from_i64(10)) + lo;
    if ((rng() & 1ULL) != 0ULL) {
      v = t81::T81BigInt::neg(v);
    }
    p.bigint_pool.push_back(v);
  }

  const auto symbol_count = static_cast<int>(rng() % 6);
  for (int i = 0; i < symbol_count; ++i) {
    p.symbol_pool.push_back(rand_ident(rng, "sym"));
  }

  const auto tensor_count = static_cast<int>(rng() % 4);
  for (int i = 0; i < tensor_count; ++i) {
    const int d0 = 1 + static_cast<int>(rng() % 3);
    const int d1 = 1 + static_cast<int>(rng() % 3);
    std::vector<float> data(static_cast<std::size_t>(d0 * d1), 0.0f);
    for (auto& v : data) {
      v = static_cast<float>(static_cast<int>(rng() % 100) - 50) / 7.0f;
    }
    p.tensor_pool.emplace_back(std::vector<int>{d0, d1}, std::move(data));
  }

  const auto shape_count = static_cast<int>(rng() % 5);
  for (int i = 0; i < shape_count; ++i) {
    p.shape_pool.push_back({1 + static_cast<int>(rng() % 4), 1 + static_cast<int>(rng() % 4)});
  }

  p.axion_policy_text = "(policy " + rand_ident(rng, "tier") + ")";
  p.match_metadata_text = "(match " + rand_ident(rng, "meta") + ")";

  const auto alias_count = static_cast<int>(rng() % 5);
  for (int i = 0; i < alias_count; ++i) {
    p.type_aliases.push_back(random_alias(rng));
  }

  const auto enum_count = static_cast<int>(rng() % 4);
  for (int i = 0; i < enum_count; ++i) {
    p.enum_metadata.push_back(random_enum_meta(rng));
  }

  return p;
}

bool program_equal(const t81::tisc::Program& a, const t81::tisc::Program& b) {
  if (a.insns.size() != b.insns.size()) return false;
  for (std::size_t i = 0; i < a.insns.size(); ++i) {
    if (a.insns[i].opcode != b.insns[i].opcode) return false;
    if (a.insns[i].a != b.insns[i].a) return false;
    if (a.insns[i].b != b.insns[i].b) return false;
    if (a.insns[i].c != b.insns[i].c) return false;
    if (a.insns[i].literal_kind != b.insns[i].literal_kind) return false;
  }

  if (a.float_pool != b.float_pool) return false;
  if (a.bigint_pool.size() != b.bigint_pool.size()) return false;
  for (std::size_t i = 0; i < a.bigint_pool.size(); ++i) {
    if (a.bigint_pool[i].to_string() != b.bigint_pool[i].to_string()) return false;
  }
  if (a.symbol_pool != b.symbol_pool) return false;
  if (a.shape_pool != b.shape_pool) return false;
  if (a.axion_policy_text != b.axion_policy_text) return false;
  if (a.match_metadata_text != b.match_metadata_text) return false;

  if (a.fraction_pool.size() != b.fraction_pool.size()) return false;
  for (std::size_t i = 0; i < a.fraction_pool.size(); ++i) {
    if (a.fraction_pool[i].to_string() != b.fraction_pool[i].to_string()) return false;
  }

  if (a.tensor_pool.size() != b.tensor_pool.size()) return false;
  for (std::size_t i = 0; i < a.tensor_pool.size(); ++i) {
    if (a.tensor_pool[i].shape() != b.tensor_pool[i].shape()) return false;
    if (a.tensor_pool[i].data() != b.tensor_pool[i].data()) return false;
  }

  if (a.type_aliases.size() != b.type_aliases.size()) return false;
  for (std::size_t i = 0; i < a.type_aliases.size(); ++i) {
    const auto& x = a.type_aliases[i];
    const auto& y = b.type_aliases[i];
    if (x.name != y.name) return false;
    if (x.params != y.params) return false;
    if (x.alias != y.alias) return false;
    if (x.kind != y.kind) return false;
    if (x.schema_version != y.schema_version) return false;
    if (x.module_path != y.module_path) return false;
    if (x.fields.size() != y.fields.size()) return false;
    for (std::size_t j = 0; j < x.fields.size(); ++j) {
      if (x.fields[j].name != y.fields[j].name) return false;
      if (x.fields[j].type != y.fields[j].type) return false;
    }
    if (x.variants.size() != y.variants.size()) return false;
    for (std::size_t j = 0; j < x.variants.size(); ++j) {
      if (x.variants[j].name != y.variants[j].name) return false;
      if (x.variants[j].payload != y.variants[j].payload) return false;
    }
  }

  if (a.enum_metadata.size() != b.enum_metadata.size()) return false;
  for (std::size_t i = 0; i < a.enum_metadata.size(); ++i) {
    const auto& x = a.enum_metadata[i];
    const auto& y = b.enum_metadata[i];
    if (x.enum_id != y.enum_id) return false;
    if (x.name != y.name) return false;
    if (x.variants.size() != y.variants.size()) return false;
    for (std::size_t j = 0; j < x.variants.size(); ++j) {
      if (x.variants[j].name != y.variants[j].name) return false;
      if (x.variants[j].payload != y.variants[j].payload) return false;
      if (x.variants[j].variant_id != y.variants[j].variant_id) return false;
    }
  }
  return true;
}

bool test_binary_metadata_roundtrip_property() {
  std::mt19937_64 rng(0xB1A2C3D481ULL);
  const fs::path base = fs::temp_directory_path() / "t81-tisc-roundtrip-property";

  for (int i = 0; i < 48; ++i) {
    const fs::path f1 = base.string() + "-" + std::to_string(i) + "-1.tisc";
    const fs::path f2 = base.string() + "-" + std::to_string(i) + "-2.tisc";
    const fs::path f3 = base.string() + "-" + std::to_string(i) + "-3.tisc";

    const auto original = random_program(rng);
    t81::tisc::save_program(original, f1.string());
    const auto l1 = t81::tisc::load_program(f1.string());
    t81::tisc::save_program(l1, f2.string());
    const auto l2 = t81::tisc::load_program(f2.string());
    t81::tisc::save_program(l2, f3.string());

    const auto b1 = read_bytes(f1);
    const auto b2 = read_bytes(f2);
    const auto b3 = read_bytes(f3);
    if (!expect(!b1.empty(), "failed to read first encoded file")) return false;
    if (!expect(!b2.empty(), "failed to read second encoded file")) return false;
    if (!expect(!b3.empty(), "failed to read third encoded file")) return false;
    if (!expect(b1 == b2, "binary drift between first and second serialization")) return false;
    if (!expect(b2 == b3, "binary drift between second and third serialization")) return false;

    if (!expect(program_equal(l1, l2), "program semantic drift after roundtrip")) return false;

    std::error_code ec;
    fs::remove(f1, ec);
    fs::remove(f2, ec);
    fs::remove(f3, ec);
  }
  return true;
}

}  // namespace

int main() { return test_binary_metadata_roundtrip_property() ? 0 : 1; }
