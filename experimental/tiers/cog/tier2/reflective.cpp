#include "t81/experimental/cog/tier2/reflective.hpp"
#include "t81/axion/reasons.hpp"
#include <algorithm>
#include <sstream>

namespace t81::cog::v2 {

namespace {
std::string json_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char c : text) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}
}  // namespace

void JustificationChain::add_step(const std::string& step) {
  steps.push_back(step);
  entries.push_back(Entry{"note", step, 0, {}});
}

void JustificationChain::add_structured_step(const std::string& kind, const std::string& text,
                                             std::size_t pc,
                                             const std::vector<std::int64_t>& register_prefix) {
  steps.push_back(text);
  entries.push_back(Entry{kind, text, pc, register_prefix});
}

void ReflectiveFrame::capture_state(std::size_t capture_pc,
                                    const std::vector<std::int64_t>& capture_registers,
                                    const std::string& capture_description) {
  pc = capture_pc;
  registers = capture_registers;
  description = capture_description;
  std::vector<std::int64_t> prefix;
  prefix.reserve(std::min<std::size_t>(4, capture_registers.size()));
  for (std::size_t i = 0; i < std::min<std::size_t>(4, capture_registers.size()); ++i) {
    prefix.push_back(capture_registers[i]);
  }
  justification.add_structured_step("capture", "Captured state: " + capture_description, capture_pc, prefix);
  sealed = false;
  hash = 0;
}

void ReflectiveFrame::add_justification_step(const std::string& text, std::size_t current_pc,
                                             const std::vector<std::int64_t>& regs) {
  if (sealed) return;
  std::vector<std::int64_t> prefix;
  prefix.reserve(std::min<std::size_t>(4, regs.size()));
  for (std::size_t i = 0; i < std::min<std::size_t>(4, regs.size()); ++i) {
    prefix.push_back(regs[i]);
  }
  justification.add_structured_step("justify", text, current_pc, prefix);
}

bool ReflectiveFrame::check(const std::string& criteria) const {
  if (description.find(criteria) != std::string::npos) return true;
  for (const auto& step : justification.steps) {
    if (step.find(criteria) != std::string::npos) return true;
  }
  return false;
}

void ReflectiveFrame::trace(std::size_t current_pc, const std::vector<std::int64_t>& regs) {
  if (sealed) return;
  std::ostringstream oss;
  oss << "Trace PC=" << current_pc;
  // Log first few registers for context
  for (size_t i = 0; i < std::min<size_t>(regs.size(), 4); ++i) {
    oss << " R" << i << "=" << regs[i];
  }
  std::vector<std::int64_t> prefix;
  prefix.reserve(std::min<std::size_t>(4, regs.size()));
  for (std::size_t i = 0; i < std::min<std::size_t>(4, regs.size()); ++i) {
    prefix.push_back(regs[i]);
  }
  justification.add_structured_step("trace", oss.str(), current_pc, prefix);
}

void ReflectiveFrame::seal() {
  if (sealed) return;
  std::uint64_t h = 0xcbf29ce484222325ULL;  // FNV offset basis
  auto mix = [&](const std::string& s) {
    for (char c : s) {
      h ^= static_cast<unsigned char>(c);
      h *= 0x100000001b3ULL;  // FNV prime
    }
  };
  mix(description);
  mix(std::to_string(pc));
  for (const auto reg : registers) {
    mix(std::to_string(reg));
  }
  for (const auto& entry : justification.entries) {
    mix(entry.kind);
    mix(entry.text);
    mix(std::to_string(entry.pc));
    for (const auto reg : entry.register_prefix) {
      mix(std::to_string(reg));
    }
  }
  hash = h;
  sealed = true;
  justification.add_structured_step("seal", "Sealed hash=" + std::to_string(h), pc, {});
}

std::string ReflectiveFrame::serialize_evidence_canonical() const {
  std::ostringstream oss;
  oss << "{";
  oss << "\"schema\":\"t81.reflective-evidence.v1\",";
  oss << "\"pc\":" << pc << ",";
  oss << "\"sealed\":" << (sealed ? "true" : "false") << ",";
  oss << "\"hash\":" << hash << ",";
  oss << "\"description\":\"" << json_escape(description) << "\",";
  oss << "\"registers\":[";
  for (std::size_t i = 0; i < registers.size(); ++i) {
    if (i != 0) oss << ",";
    oss << registers[i];
  }
  oss << "],";
  oss << "\"evidence\":[";
  for (std::size_t i = 0; i < justification.entries.size(); ++i) {
    const auto& entry = justification.entries[i];
    if (i != 0) oss << ",";
    oss << "{";
    oss << "\"kind\":\"" << json_escape(entry.kind) << "\",";
    oss << "\"text\":\"" << json_escape(entry.text) << "\",";
    oss << "\"pc\":" << entry.pc << ",";
    oss << "\"register_prefix\":[";
    for (std::size_t r = 0; r < entry.register_prefix.size(); ++r) {
      if (r != 0) oss << ",";
      oss << entry.register_prefix[r];
    }
    oss << "]";
    oss << "}";
  }
  oss << "]";
  oss << "}";
  return oss.str();
}

bool ReflectiveFrame::verify_evidence_replay(const ReflectiveFrame& expected,
                                             const ReflectiveFrame& actual, std::string* reason) {
  if (expected.serialize_evidence_canonical() == actual.serialize_evidence_canonical()) {
    if (reason) *reason = "evidence-identical";
    return true;
  }
  if (expected.description != actual.description) {
    if (reason) *reason = "description-mismatch";
    return false;
  }
  if (expected.pc != actual.pc) {
    if (reason) *reason = "pc-mismatch";
    return false;
  }
  if (expected.hash != actual.hash) {
    if (reason) *reason = "hash-mismatch";
    return false;
  }
  if (expected.registers != actual.registers) {
    if (reason) *reason = "register-snapshot-mismatch";
    return false;
  }
  if (expected.justification.entries.size() != actual.justification.entries.size()) {
    if (reason) *reason = "evidence-size-mismatch";
    return false;
  }
  for (std::size_t i = 0; i < expected.justification.entries.size(); ++i) {
    const auto& left = expected.justification.entries[i];
    const auto& right = actual.justification.entries[i];
    if (left.kind != right.kind || left.text != right.text || left.pc != right.pc ||
        left.register_prefix != right.register_prefix) {
      if (reason) *reason = "evidence-entry-mismatch@" + std::to_string(i);
      return false;
    }
  }
  if (reason) *reason = "unknown-mismatch";
  return false;
}

std::string ReflectiveFrame::axion_trace_event(std::string_view action) const {
  std::ostringstream oss;
  oss << t81::axion::reasons::kCogTier2Reflect;
  oss << " action=" << action;
  oss << " pc=" << pc;
  oss << " steps=" << justification.steps.size();
  oss << " sealed=" << (sealed ? 1 : 0);
  if (sealed) {
    oss << " hash=" << hash;
  }
  if (!description.empty()) {
    oss << " desc=\"" << description << "\"";
  }
  return oss.str();
}

}  // namespace t81::cog::v2
