#include "t81/isa/binary_io.hpp"
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "t81/bigint.hpp"
#include "t81/fraction.hpp"
#include "t81/isa/type_alias.hpp"
#include "t81/tensor.hpp"
#include "t81/types/T81Complex.hpp"

namespace t81 {
namespace tisc {

// Maximum number of elements allowed when deserialising a length-prefixed
// field. This prevents a corrupt or truncated file from triggering a
// multi-gigabyte allocation and OOM-kill.
static constexpr uint64_t kMaxDeserialiseCount = 1u << 24;  // 16 million elements

static uint64_t read_checked_size(std::istream& is) {
  uint64_t size = 0;
  is.read(reinterpret_cast<char*>(&size), sizeof(size));
  if (!is) {
    throw std::runtime_error("TISC: unexpected end of file reading length prefix");
  }
  if (size > kMaxDeserialiseCount) {
    throw std::runtime_error("TISC: corrupt file — unreasonable element count " +
                             std::to_string(size));
  }
  return size;
}

// --- Generic Helpers for Simple Types ---
template <typename T>
void write_vector(std::ostream& os, const std::vector<T>& vec) {
  uint64_t size = vec.size();
  os.write(reinterpret_cast<const char*>(&size), sizeof(size));
  if (size > 0) {
    os.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
  }
}

template <typename T>
void read_vector(std::istream& is, std::vector<T>& vec) {
  const uint64_t size = read_checked_size(is);
  vec.resize(size);
  if (size > 0) {
    is.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
  }
}

void write_insn_vector(std::ostream& os, const std::vector<t81::tisc::Insn>& vec) {
  uint64_t size = vec.size();
  os.write(reinterpret_cast<const char*>(&size), sizeof(size));
  for (const auto& insn : vec) {
    const auto opcode = static_cast<uint8_t>(insn.opcode);
    os.write(reinterpret_cast<const char*>(&opcode), sizeof(opcode));
    os.write(reinterpret_cast<const char*>(&insn.a), sizeof(insn.a));
    os.write(reinterpret_cast<const char*>(&insn.b), sizeof(insn.b));
    os.write(reinterpret_cast<const char*>(&insn.c), sizeof(insn.c));
    const auto literal_kind = static_cast<uint8_t>(insn.literal_kind);
    os.write(reinterpret_cast<const char*>(&literal_kind), sizeof(literal_kind));
  }
}

void read_insn_vector(std::istream& is, std::vector<t81::tisc::Insn>& vec) {
  const uint64_t size = read_checked_size(is);
  vec.resize(size);
  for (auto& insn : vec) {
    uint8_t opcode = 0;
    is.read(reinterpret_cast<char*>(&opcode), sizeof(opcode));
    insn.opcode = static_cast<t81::tisc::Opcode>(opcode);
    is.read(reinterpret_cast<char*>(&insn.a), sizeof(insn.a));
    is.read(reinterpret_cast<char*>(&insn.b), sizeof(insn.b));
    is.read(reinterpret_cast<char*>(&insn.c), sizeof(insn.c));
    uint8_t literal_kind = 0;
    is.read(reinterpret_cast<char*>(&literal_kind), sizeof(literal_kind));
    insn.literal_kind = static_cast<t81::tisc::LiteralKind>(literal_kind);
  }
}

// --- Specializations for Complex Types ---

// std::string
void write_string(std::ostream& os, const std::string& str) {
  uint64_t size = str.size();
  os.write(reinterpret_cast<const char*>(&size), sizeof(size));
  os.write(str.data(), size);
}

void read_string(std::istream& is, std::string& str) {
  const uint64_t size = read_checked_size(is);
  str.resize(size);
  if (size > 0) {
    is.read(&str[0], size);
  }
}

template <typename T>
void write_serializable_vector(std::ostream& os, const std::vector<T>& vec) {
  uint64_t size = vec.size();
  os.write(reinterpret_cast<const char*>(&size), sizeof(size));
  for (const auto& item : vec) {
    item.serialize(os);
  }
}

template <typename T>
void read_serializable_vector(std::istream& is, std::vector<T>& vec) {
  const uint64_t size = read_checked_size(is);
  vec.resize(size);
  for (uint64_t i = 0; i < size; ++i) {
    vec[i].deserialize(is);
  }
}

void write_vector_string(std::ostream& os, const std::vector<std::string>& vec) {
  uint64_t size = vec.size();
  os.write(reinterpret_cast<const char*>(&size), sizeof(size));
  for (const auto& str : vec) {
    write_string(os, str);
  }
}

void read_vector_string(std::istream& is, std::vector<std::string>& vec) {
  const uint64_t size = read_checked_size(is);
  vec.resize(size);
  for (uint64_t i = 0; i < size; ++i) {
    read_string(is, vec[i]);
  }
}

// vector<vector<int>> for shapes
void write_vector_vector_int(std::ostream& os, const std::vector<std::vector<int>>& vec) {
  uint64_t size = vec.size();
  os.write(reinterpret_cast<const char*>(&size), sizeof(size));
  for (const auto& inner_vec : vec) {
    write_vector(os, inner_vec);
  }
}

void read_vector_vector_int(std::istream& is, std::vector<std::vector<int>>& vec) {
  const uint64_t size = read_checked_size(is);
  vec.resize(size);
  for (uint64_t i = 0; i < size; ++i) {
    read_vector(is, vec[i]);
  }
}

static void write_field_info(std::ostream& os, const t81::tisc::FieldInfo& field) {
  write_string(os, field.name);
  write_string(os, field.type);
}

static void read_field_info(std::istream& is, t81::tisc::FieldInfo& field) {
  read_string(is, field.name);
  read_string(is, field.type);
}

static void write_variant_info(std::ostream& os, const t81::tisc::VariantInfo& variant) {
  write_string(os, variant.name);
  uint8_t has_payload = variant.payload.has_value() ? 1 : 0;
  os.write(reinterpret_cast<const char*>(&has_payload), sizeof(has_payload));
  if (variant.payload.has_value()) {
    write_string(os, *variant.payload);
  }
}

static void read_variant_info(std::istream& is, t81::tisc::VariantInfo& variant) {
  read_string(is, variant.name);
  uint8_t has_payload = 0;
  is.read(reinterpret_cast<char*>(&has_payload), sizeof(has_payload));
  if (has_payload) {
    std::string payload;
    read_string(is, payload);
    variant.payload = std::move(payload);
  } else {
    variant.payload.reset();
  }
}

static void write_enum_variant_metadata(std::ostream& os,
                                        const t81::tisc::EnumVariantMetadata& variant) {
  write_string(os, variant.name);
  uint8_t has_payload = variant.payload.has_value() ? 1 : 0;
  os.write(reinterpret_cast<const char*>(&has_payload), sizeof(has_payload));
  if (variant.payload.has_value()) {
    write_string(os, *variant.payload);
  }
  os.write(reinterpret_cast<const char*>(&variant.variant_id), sizeof(variant.variant_id));
}

static void read_enum_variant_metadata(std::istream& is, t81::tisc::EnumVariantMetadata& variant) {
  read_string(is, variant.name);
  uint8_t has_payload = 0;
  is.read(reinterpret_cast<char*>(&has_payload), sizeof(has_payload));
  if (has_payload) {
    std::string payload;
    read_string(is, payload);
    variant.payload = std::move(payload);
  } else {
    variant.payload.reset();
  }
  is.read(reinterpret_cast<char*>(&variant.variant_id), sizeof(variant.variant_id));
}

static void write_enum_metadata(std::ostream& os, const t81::tisc::EnumMetadata& meta) {
  os.write(reinterpret_cast<const char*>(&meta.enum_id), sizeof(meta.enum_id));
  write_string(os, meta.name);
  uint64_t variant_count = meta.variants.size();
  os.write(reinterpret_cast<const char*>(&variant_count), sizeof(variant_count));
  for (const auto& variant : meta.variants) {
    write_enum_variant_metadata(os, variant);
  }
}

static void read_enum_metadata(std::istream& is, t81::tisc::EnumMetadata& meta) {
  is.read(reinterpret_cast<char*>(&meta.enum_id), sizeof(meta.enum_id));
  read_string(is, meta.name);
  const uint64_t variant_count = read_checked_size(is);
  meta.variants.resize(variant_count);
  for (auto& variant : meta.variants) {
    read_enum_variant_metadata(is, variant);
  }
}

static void write_type_alias_metadata(std::ostream& os, const t81::tisc::TypeAliasMetadata& meta) {
  write_string(os, meta.name);
  write_vector_string(os, meta.params);
  write_string(os, meta.alias);
  uint8_t kind = static_cast<uint8_t>(meta.kind);
  os.write(reinterpret_cast<const char*>(&kind), sizeof(kind));
  uint32_t schema = meta.schema_version;
  os.write(reinterpret_cast<const char*>(&schema), sizeof(schema));
  write_string(os, meta.module_path);

  uint64_t field_count = meta.fields.size();
  os.write(reinterpret_cast<const char*>(&field_count), sizeof(field_count));
  for (const auto& field : meta.fields) {
    write_field_info(os, field);
  }

  uint64_t variant_count = meta.variants.size();
  os.write(reinterpret_cast<const char*>(&variant_count), sizeof(variant_count));
  for (const auto& variant : meta.variants) {
    write_variant_info(os, variant);
  }
}

static void read_type_alias_metadata(std::istream& is, t81::tisc::TypeAliasMetadata& meta) {
  read_string(is, meta.name);
  read_vector_string(is, meta.params);
  read_string(is, meta.alias);
  uint8_t kind = 0;
  is.read(reinterpret_cast<char*>(&kind), sizeof(kind));
  meta.kind = static_cast<t81::tisc::StructuralKind>(kind);
  uint32_t schema = 1;
  is.read(reinterpret_cast<char*>(&schema), sizeof(schema));
  meta.schema_version = schema;
  read_string(is, meta.module_path);

  const uint64_t field_count = read_checked_size(is);
  meta.fields.resize(field_count);
  for (auto& field : meta.fields) {
    read_field_info(is, field);
  }

  const uint64_t variant_count = read_checked_size(is);
  meta.variants.resize(variant_count);
  for (auto& variant : meta.variants) {
    read_variant_info(is, variant);
  }
}

// --- Main Functions ---

void save_program(const Program& program, const std::string& path) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file for writing: " + path);
  }

  write_insn_vector(file, program.insns);
  write_vector(file, program.float_pool);
  write_serializable_vector(file, program.fraction_pool);
  write_vector_string(file, program.symbol_pool);
  write_serializable_vector(file, program.tensor_pool);
  write_vector_vector_int(file, program.shape_pool);
  write_serializable_vector(file, program.complex_pool);
  write_string(file, program.axion_policy_text);
  uint64_t alias_count = program.type_aliases.size();
  file.write(reinterpret_cast<const char*>(&alias_count), sizeof(alias_count));
  for (const auto& alias : program.type_aliases) {
    write_type_alias_metadata(file, alias);
  }
  write_string(file, program.match_metadata_text);
  uint64_t enum_count = program.enum_metadata.size();
  file.write(reinterpret_cast<const char*>(&enum_count), sizeof(enum_count));
  for (const auto& enum_meta : program.enum_metadata) {
    write_enum_metadata(file, enum_meta);
  }
  if (!program.bigint_pool.empty()) {
    write_serializable_vector(file, program.bigint_pool);
  }
}

Program load_program(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file for reading: " + path);
  }

  Program program;
  read_insn_vector(file, program.insns);
  read_vector(file, program.float_pool);
  read_serializable_vector(file, program.fraction_pool);
  read_vector_string(file, program.symbol_pool);
  read_serializable_vector(file, program.tensor_pool);
  read_vector_vector_int(file, program.shape_pool);
  read_serializable_vector(file, program.complex_pool);
  read_string(file, program.axion_policy_text);

  if (file.peek() != std::char_traits<char>::eof()) {
    const uint64_t alias_count = read_checked_size(file);
    program.type_aliases.resize(alias_count);
    for (auto& alias : program.type_aliases) {
      read_type_alias_metadata(file, alias);
    }
    if (file.peek() != std::char_traits<char>::eof()) {
      read_string(file, program.match_metadata_text);
    }
    if (file.peek() != std::char_traits<char>::eof()) {
      const uint64_t enum_count = read_checked_size(file);
      program.enum_metadata.resize(enum_count);
      for (auto& enum_meta : program.enum_metadata) {
        read_enum_metadata(file, enum_meta);
      }
    }
    if (file.peek() != std::char_traits<char>::eof()) {
      read_serializable_vector(file, program.bigint_pool);
    }
  }

  return program;
}

}  // namespace tisc
}  // namespace t81
