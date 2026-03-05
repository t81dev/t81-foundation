#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace t81::cog::v2 {

struct JustificationChain {
  struct Entry {
    std::string kind;
    std::string text;
    std::size_t pc{0};
    std::vector<std::int64_t> register_prefix;
  };

  std::vector<std::string> steps;
  std::vector<Entry> entries;

  void add_step(const std::string& step);
  void add_structured_step(const std::string& kind, const std::string& text, std::size_t pc,
                           const std::vector<std::int64_t>& register_prefix = {});
};

struct ReflectiveFrame {
  std::size_t pc{0};
  std::vector<std::int64_t> registers;
  std::string description;
  JustificationChain justification;
  bool sealed{false};
  std::uint64_t hash{0};

  void capture_state(std::size_t pc, const std::vector<std::int64_t>& registers,
                     const std::string& description);
  void add_justification_step(const std::string& text, std::size_t current_pc = 0,
                              const std::vector<std::int64_t>& regs = {});
  bool check(const std::string& criteria) const;
  void trace(std::size_t current_pc, const std::vector<std::int64_t>& regs);
  void seal();
  std::string serialize_evidence_canonical() const;
  static bool verify_evidence_replay(const ReflectiveFrame& expected, const ReflectiveFrame& actual,
                                     std::string* reason = nullptr);
  std::string axion_trace_event(std::string_view action) const;
};

}  // namespace t81::cog::v2
