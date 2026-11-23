#include "t81/axion/nondeterminism_detector.hpp"

#include <algorithm>
#include <cstdint>

#include "t81/crypto/sha3.hpp"

namespace t81::axion {

namespace {

void append_le32(std::vector<std::uint8_t>& out, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xffu));
  }
}

void append_le64(std::vector<std::uint8_t>& out, std::uint64_t v) {
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xffu));
  }
}

}  // namespace

// static
std::size_t DeterminismDetector::hash_event(const t81::vm::AxionEvent& ev, std::size_t seed) {
  // Canonical little-endian framing -> SHA3-512 -> truncated 64-bit state.
  std::vector<std::uint8_t> buf;
  buf.reserve(24 + ev.verdict.reason.size());
  append_le64(buf, static_cast<std::uint64_t>(seed));
  append_le32(buf, static_cast<std::uint32_t>(ev.opcode));
  append_le32(buf, static_cast<std::uint32_t>(ev.tag));
  append_le32(buf, static_cast<std::uint32_t>(ev.verdict.kind));
  append_le32(buf, static_cast<std::uint32_t>(ev.verdict.reason.size()));
  buf.insert(buf.end(), ev.verdict.reason.begin(), ev.verdict.reason.end());
  const auto digest = t81::crypto::sha3_512(buf);
  std::uint64_t out = 0;
  for (int i = 0; i < 8; ++i) {
    out |= static_cast<std::uint64_t>(digest[static_cast<std::size_t>(i)]) << (i * 8);
  }
  return static_cast<std::size_t>(out);
}

void DeterminismDetector::record_run(const std::vector<t81::vm::AxionEvent>& log) {
  prev_hashes_ = std::move(curr_hashes_);
  curr_hashes_.clear();
  curr_hashes_.reserve(log.size());
  std::size_t rolling = 0;
  for (const auto& ev : log) {
    rolling = hash_event(ev, rolling);
    curr_hashes_.push_back(rolling);
  }
}

DivergenceReport DeterminismDetector::check_against_previous() const {
  if (prev_hashes_.empty()) {
    return DivergenceReport{false, 0, {}};
  }

  const std::size_t min_len = std::min(curr_hashes_.size(), prev_hashes_.size());
  for (std::size_t i = 0; i < min_len; ++i) {
    if (curr_hashes_[i] != prev_hashes_[i]) {
      return DivergenceReport{
          true, i, "Nondeterministic divergence at event " + std::to_string(i)};
    }
  }

  if (curr_hashes_.size() != prev_hashes_.size()) {
    const std::size_t idx = min_len;
    return DivergenceReport{
        true, idx,
        "Nondeterministic divergence at event " + std::to_string(idx) +
            ": event count mismatch (prev=" + std::to_string(prev_hashes_.size()) +
            " curr=" + std::to_string(curr_hashes_.size()) + ")"};
  }

  return DivergenceReport{false, 0, {}};
}

}  // namespace t81::axion
