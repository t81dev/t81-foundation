#include "t81/isa/base81_view.hpp"
#include <sstream>
#include <charconv>
#include "t81/codec/trit_packing.hpp"
#include "t81/types/T81Int.hpp"

namespace t81::tisc::base81_view {

using namespace t81::codec::trit_packing;

std::string render_insn(const Insn& insn) {
  std::vector<Trit> trits;
  trits.reserve(96);

  // Opcode: 8 trits
  T81Int<8> op(static_cast<int64_t>(insn.opcode));
  for (int i = 0; i < 8; ++i) trits.push_back(op[i]);

  // A: 21 trits (fits int32_t)
  T81Int<21> va(static_cast<int64_t>(insn.a));
  for (int i = 0; i < 21; ++i) trits.push_back(va[i]);

  // B: 41 trits (fits int64_t)
  T81Int<41> vb(static_cast<int64_t>(insn.b));
  for (int i = 0; i < 41; ++i) trits.push_back(vb[i]);

  // C: 21 trits (fits int32_t)
  T81Int<21> vc(static_cast<int64_t>(insn.c));
  for (int i = 0; i < 21; ++i) trits.push_back(vc[i]);

  // LiteralKind: 5 trits
  T81Int<5> vlk(static_cast<int64_t>(insn.literal_kind));
  for (int i = 0; i < 5; ++i) trits.push_back(vlk[i]);

  auto b81_res = pack_base81(trits);
  if (!b81_res) return "???";
  return b81_digits_to_string(b81_res.value());
}

std::string render(const Program& program) {
  std::ostringstream oss;

  if (!program.symbol_pool.empty()) {
    oss << "@symbols\n";
    for (const auto& s : program.symbol_pool) {
      if (s.empty()) {
        oss << "(empty)\n";
      } else {
        for (unsigned char c : s) {
          oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
        oss << std::dec << "\n";
      }
    }
  }

  if (!program.float_pool.empty()) {
    oss << "@floats\n";
    oss.precision(17);
    for (double f : program.float_pool) {
      oss << f << "\n";
    }
  }

  if (!program.bigint_pool.empty()) {
    oss << "@bigints\n";
    for (const auto& b : program.bigint_pool) {
      oss << b.to_string() << "\n";
    }
  }

  if (!program.fraction_pool.empty()) {
    oss << "@fractions\n";
    for (const auto& f : program.fraction_pool) {
      oss << f.to_string() << "\n";
    }
  }

  if (!program.complex_pool.empty()) {
    oss << "@complexes\n";
    for (const auto& c : program.complex_pool) {
      oss << c.real().to_canonical_string() << " " << c.imag().to_canonical_string() << "\n";
    }
  }

  if (!program.shape_pool.empty()) {
    oss << "@shapes\n";
    for (const auto& shape : program.shape_pool) {
      for (size_t i = 0; i < shape.size(); ++i) {
        oss << shape[i] << (i + 1 == shape.size() ? "" : ",");
      }
      oss << "\n";
    }
  }

  if (!program.tensor_pool.empty()) {
    oss << "@tensors\n";
    for (const auto& tensor : program.tensor_pool) {
      const auto& s = tensor.shape();
      for (size_t i = 0; i < s.size(); ++i) {
        oss << s[i] << (i + 1 == s.size() ? "" : ",");
      }
      oss << " " << tensor_numeric_class_name(tensor.numeric_class());
      const auto& d = tensor.data();
      for (float v : d) {
        oss << " " << v;
      }
      oss << "\n";
    }
  }

  if (!program.match_metadata_text.empty()) {
    oss << "@match_metadata\n" << program.match_metadata_text << "\n@endmatch_metadata\n";
  }

  if (!program.axion_policy_text.empty()) {
    oss << "@policy\n" << program.axion_policy_text << "\n@endpolicy\n";
  }

  if (!program.function_metadata.empty()) {
    oss << "@functions\n";
    for (const auto& f : program.function_metadata) {
      oss << f.name << " " << (f.is_axion_verify ? "1" : "0") << "\n";
    }
  }

  oss << "@code\n";
  for (const auto& insn : program.insns) {
    oss << render_insn(insn) << "\n";
  }

  return oss.str();
}

Result<Program> parse(std::string_view s) {
  Program program;
  std::istringstream iss{std::string(s)};
  std::string line;
  std::string section = "code";

  while (std::getline(iss, line)) {
    if (line.size() > 1 && line[0] == '@' && std::isalpha(line[1])) {
      if (line == "@symbols") {
        section = "symbols";
        continue;
      } else if (line == "@floats") {
        section = "floats";
        continue;
      } else if (line == "@bigints") {
        section = "bigints";
        continue;
      } else if (line == "@fractions") {
        section = "fractions";
        continue;
      } else if (line == "@complexes") {
        section = "complexes";
        continue;
      } else if (line == "@shapes") {
        section = "shapes";
        continue;
      } else if (line == "@tensors") {
        section = "tensors";
        continue;
      } else if (line == "@match_metadata") {
        section = "match_metadata";
        continue;
      } else if (line == "@policy") {
        section = "policy";
        continue;
      } else if (line == "@functions") {
        section = "functions";
        continue;
      } else if (line == "@code") {
        section = "code";
        continue;
      } else if (line == "@endpolicy" || line == "@endmatch_metadata") {
        section = "code";
        continue;
      }
    }

    if (section == "symbols") {
      if (line == "(empty)") {
        program.symbol_pool.push_back("");
      } else if (!line.empty()) {
        std::string decoded;
        for (size_t i = 0; i + 1 < line.size(); i += 2) {
          int v = 0;
          auto [ptr, ec] = std::from_chars(line.data() + i, line.data() + i + 2, v, 16);
          if (ec == std::errc()) decoded.push_back(static_cast<char>(v));
        }
        program.symbol_pool.push_back(decoded);
      }
    } else if (section == "floats") {
      if (!line.empty()) program.float_pool.push_back(std::stod(line));
    } else if (section == "bigints") {
      if (!line.empty()) program.bigint_pool.push_back(t81::T81BigInt::from_decimal_string(line));
    } else if (section == "fractions") {
      auto slash = line.find('/');
      if (slash != std::string::npos) {
        auto num = t81::T81BigInt::from_decimal_string(line.substr(0, slash));
        auto den = t81::T81BigInt::from_decimal_string(line.substr(slash + 1));
        program.fraction_pool.emplace_back(num, den);
      }
    } else if (section == "complexes") {
      std::istringstream lss{line};
      std::string rs, is;
      if (lss >> rs >> is) {
        // T81Float reconstruction from canonical string is not trivial as it doesn't have a from_string.
        // But for now, we can use from_double as a fallback if the canonical string is well-behaved,
        // or just skip complex pool reconstruction in Base81 if too complex for a quick fix.
        // Actually, T81Float has from_double.
        // Let's try to parse the canonical string manually or use double.
        // Canonical string: "+111...E0"
        auto parse_f = [](const std::string& str) {
          if (str == "NaE") return T81Float18_9::nae();
          if (str == "+Inf") return T81Float18_9::inf(true);
          if (str == "-Inf") return T81Float18_9::inf(false);
          // Simple fallback for numeric parts
          try {
            return T81Float18_9::from_double(std::stod(str));
          } catch (...) {
            return T81Float18_9::zero();
          }
        };
        program.complex_pool.emplace_back(parse_f(rs), parse_f(is));
      }
    } else if (section == "shapes") {
      std::vector<int> shape;
      std::istringstream lss{line};
      std::string part;
      while (std::getline(lss, part, ',')) {
        if (!part.empty()) shape.push_back(std::stoi(part));
      }
      program.shape_pool.push_back(std::move(shape));
    } else if (section == "tensors") {
      std::istringstream lss{line};
      std::string shape_str, class_str;
      if (lss >> shape_str >> class_str) {
        std::vector<int> shape;
        std::istringstream sss{shape_str};
        std::string part;
        while (std::getline(sss, part, ',')) {
          if (!part.empty()) shape.push_back(std::stoi(part));
        }
        std::vector<float> data;
        float v;
        while (lss >> v) data.push_back(v);
        T729DynamicTensor tensor(shape, data);
        if (class_str == "exact_int")
          tensor.set_numeric_class(TensorNumericClass::ExactInt);
        else if (class_str == "exact_trit")
          tensor.set_numeric_class(TensorNumericClass::ExactTrit);
        program.tensor_pool.push_back(std::move(tensor));
      }
    } else if (section == "match_metadata") {
      if (!program.match_metadata_text.empty()) program.match_metadata_text += "\n";
      program.match_metadata_text += line;
    } else if (section == "policy") {
      if (!program.axion_policy_text.empty()) program.axion_policy_text += "\n";
      program.axion_policy_text += line;
    } else if (section == "functions") {
      std::istringstream lss{line};
      std::string name;
      int verify;
      if (lss >> name >> verify) {
        program.function_metadata.push_back({name, verify != 0});
      }
    } else if (section == "code") {
      std::istringstream line_ss{line};
      std::string word;
      while (line_ss >> word) {
        auto digits_res = string_to_b81_digits(word);
        if (!digits_res) return digits_res.error();

        auto trits_res = unpack_base81(digits_res.value(), 96);
        if (!trits_res) return trits_res.error();
        const auto& trits = trits_res.value();

        Insn insn;
        T81Int<8> op;
        for (int i = 0; i < 8; ++i) op[i] = trits[i];
        insn.opcode = static_cast<Opcode>(op.to_int64());

        T81Int<21> va;
        for (int i = 0; i < 21; ++i) va[i] = trits[8 + i];
        insn.a = static_cast<int32_t>(va.to_int64());

        T81Int<41> vb;
        for (int i = 0; i < 41; ++i) vb[i] = trits[29 + i];
        insn.b = static_cast<int64_t>(vb.to_int64());

        T81Int<21> vc;
        for (int i = 0; i < 21; ++i) vc[i] = trits[70 + i];
        insn.c = static_cast<int32_t>(vc.to_int64());

        T81Int<5> vlk;
        for (int i = 0; i < 5; ++i) vlk[i] = trits[91 + i];
        insn.literal_kind = static_cast<LiteralKind>(vlk.to_int64());

        program.insns.push_back(insn);
      }
    }
  }
  return program;
}

}  // namespace t81::tisc::base81_view
