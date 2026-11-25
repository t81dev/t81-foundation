#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "metal_dispatch.hpp"
#include "vm/regime_dispatch.hpp"

namespace {

using namespace t81::vm;

enum class KernelExecKind {
  DualRailDense,
  Sbdr128Simd,
  Int8Sparse128,
  Sbdr1t2d,
};

KernelExecKind parse_kernel_kind(std::string_view name) noexcept {
  if (name == "SBDR128_SIMD") return KernelExecKind::Sbdr128Simd;
  if (name == "INT8_Sparse128") return KernelExecKind::Int8Sparse128;
  if (name == "SBDR1T2D") return KernelExecKind::Sbdr1t2d;
  return KernelExecKind::DualRailDense;
}

std::uint64_t fnv1a_64(const std::vector<std::int64_t>& data) noexcept {
  std::uint64_t h = 14695981039346656037ULL;
  for (std::int64_t v : data) {
    const auto u = static_cast<std::uint64_t>(v);
    for (int i = 0; i < 8; ++i) {
      h ^= ((u >> (i * 8)) & 0xFFULL);
      h *= 1099511628211ULL;
    }
  }
  return h;
}

std::string hex64(std::uint64_t v) {
  std::ostringstream os;
  os << std::hex << std::setfill('0') << std::setw(16) << v;
  return os.str();
}

struct DualRailWord {
  std::uint64_t pos{0};
  std::uint64_t neg{0};
};

struct Sbdr128Block {
  std::uint32_t k_base{0};
  std::uint64_t pos_lo{0};
  std::uint64_t pos_hi{0};
  std::uint64_t neg_lo{0};
  std::uint64_t neg_hi{0};
};

// Deterministic ternary matrix-batch workload with bit-exact int64 accumulation.
// Pre-encodes the exact same logical ternary weights into each named kernel layout
// (DualRail_Dense, SBDR128_SIMD, INT8_Sparse128, SBDR1T2D) and allocates a tier-sized
// weight bank slab so working_set_bytes reflects bytes actually allocated and touched.
struct WorkloadBuffers {
  std::uint32_t out_dim{64};
  std::uint32_t k_dim{256};
  std::uint32_t words_per_row{4};  // k_dim / 64
  std::uint32_t num_banks{1};
  std::uint64_t actual_working_set_bytes{0};

  // Banked dense ternary weights [num_banks * out_dim * k_dim], values in {-1, 0, +1}.
  std::vector<std::int8_t> weights;
  // Activations [batch * k_dim], values in {-1, 0, +1}.
  std::vector<std::int8_t> activations;

  // 1) DualRail_Dense: packed (pos, neg) 64-bit bitplanes per bank & row.
  std::vector<DualRailWord> dual_rail;  // [num_banks * out_dim * words_per_row]

  // 2) SBDR128_SIMD: sparse 128-element dual-rail blocks per bank & row.
  std::vector<std::uint32_t> sbdr128_row_offsets;  // [num_banks * out_dim + 1]
  std::vector<Sbdr128Block> sbdr128_blocks;

  // 3) INT8_Sparse128: CSR non-zero int8 weights per bank & row.
  std::vector<std::uint32_t> csr_row_offsets;  // [num_banks * out_dim + 1]
  std::vector<std::uint16_t> csr_cols;
  std::vector<std::int8_t> csr_vals;

  // 4) SBDR1T2D: 2D-tiled (8 output rows x 64 k) dual-rail layout per bank.
  // Reuses dual_rail with tiled (out_tile=8, k_word) traversal order.
};

std::uint64_t tier_target_bytes(const std::string& tier, const HardwareProfile& hw, bool smoke) {
  if (smoke) {
    // Bounded working-set tiers for fast CI smoke runs; working_set_bytes in output
    // strictly reports the actual allocated/touched bytes rather than nominal hardware LLC.
    if (tier == "L2_RESIDENT") return 256ULL * 1024ULL;
    if (tier == "L3_RESIDENT") return 1ULL * 1024ULL * 1024ULL;
    if (tier == "OFF_L3_STREAMING") return 4ULL * 1024ULL * 1024ULL;
    return 8ULL * 1024ULL * 1024ULL;  // OUT_OF_CORE
  }
  const std::uint64_t llc =
      std::max<std::uint64_t>(4ULL * 1024ULL * 1024ULL, hw.effective_llc_bytes());
  constexpr std::uint64_t kMaxBenchSlabBytes = 64ULL * 1024ULL * 1024ULL;
  std::uint64_t raw = llc * 3ULL;
  if (tier == "L2_RESIDENT") {
    raw = std::max<std::uint64_t>(1024ULL * 1024ULL, hw.l2_cache_bytes / 2ULL);
  } else if (tier == "L3_RESIDENT") {
    raw = (llc * 60ULL) / 100ULL;
  } else if (tier == "OFF_L3_STREAMING") {
    raw = (llc * 120ULL) / 100ULL;
  }
  return std::min(raw, kMaxBenchSlabBytes);
}

WorkloadBuffers build_buffers(std::uint32_t batch, std::uint32_t out_dim, std::uint32_t k_dim,
                              std::uint32_t sparsity_pct, std::uint64_t seed,
                              std::uint64_t target_ws_bytes, KernelExecKind kernel_kind) {
  WorkloadBuffers buf;
  buf.out_dim = out_dim;
  buf.k_dim = k_dim;
  buf.words_per_row = k_dim / 64U;

  const std::uint64_t single_matrix_bytes = static_cast<std::uint64_t>(out_dim) * k_dim;
  const std::uint64_t act_bytes = static_cast<std::uint64_t>(batch) * k_dim;
  const std::uint64_t out_bytes =
      static_cast<std::uint64_t>(batch) * out_dim * sizeof(std::int64_t);
  const std::uint64_t fixed_bytes = act_bytes + out_bytes;
  const std::uint64_t weight_budget =
      (target_ws_bytes > fixed_bytes) ? (target_ws_bytes - fixed_bytes) : single_matrix_bytes;
  const std::uint64_t banks_u64 =
      std::max<std::uint64_t>(1ULL, (weight_budget + single_matrix_bytes - 1ULL) /
                                        std::max<std::uint64_t>(1ULL, single_matrix_bytes));
  buf.num_banks = static_cast<std::uint32_t>(std::min<std::uint64_t>(banks_u64, 1024ULL));

  const std::size_t total_weights = static_cast<std::size_t>(buf.num_banks) * out_dim * k_dim;
  buf.weights.resize(total_weights);
  buf.activations.resize(static_cast<std::size_t>(batch) * k_dim);

  std::uint64_t s = seed ^ 0x9E3779B97F4A7C15ULL;
  auto next_u64 = [&]() -> std::uint64_t {
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    return s * 2685821657736338717ULL;
  };

  for (std::size_t i = 0; i < total_weights; ++i) {
    const std::uint64_t r = next_u64();
    if ((r % 100ULL) < sparsity_pct) {
      buf.weights[i] = 0;
    } else {
      buf.weights[i] =
          ((r >> 8) & 1ULL) ? static_cast<std::int8_t>(1) : static_cast<std::int8_t>(-1);
    }
  }
  for (std::size_t i = 0; i < buf.activations.size(); ++i) {
    const std::uint64_t r = next_u64();
    const int v = static_cast<int>(r % 3ULL) - 1;
    buf.activations[i] = static_cast<std::int8_t>(v);
  }

  const std::size_t total_rows = static_cast<std::size_t>(buf.num_banks) * out_dim;
  std::uint64_t kernel_struct_bytes = 0;

  if (kernel_kind == KernelExecKind::DualRailDense || kernel_kind == KernelExecKind::Sbdr1t2d) {
    buf.dual_rail.assign(total_rows * buf.words_per_row, DualRailWord{});
    for (std::size_t r = 0; r < total_rows; ++r) {
      const std::int8_t* wrow = buf.weights.data() + r * k_dim;
      DualRailWord* drow = buf.dual_rail.data() + r * buf.words_per_row;
      for (std::uint32_t widx = 0; widx < buf.words_per_row; ++widx) {
        std::uint64_t pos = 0;
        std::uint64_t neg = 0;
        const std::uint32_t base = widx * 64U;
        for (std::uint32_t bit = 0; bit < 64U; ++bit) {
          const std::int8_t v = wrow[base + bit];
          if (v > 0) {
            pos |= (1ULL << bit);
          } else if (v < 0) {
            neg |= (1ULL << bit);
          }
        }
        drow[widx] = DualRailWord{pos, neg};
      }
    }
    kernel_struct_bytes = buf.dual_rail.size() * sizeof(DualRailWord);
  } else if (kernel_kind == KernelExecKind::Sbdr128Simd) {
    buf.sbdr128_row_offsets.resize(total_rows + 1, 0U);
    buf.sbdr128_blocks.reserve(total_rows * (k_dim / 128U));
    for (std::size_t r = 0; r < total_rows; ++r) {
      buf.sbdr128_row_offsets[r] = static_cast<std::uint32_t>(buf.sbdr128_blocks.size());
      const std::int8_t* wrow = buf.weights.data() + r * k_dim;
      for (std::uint32_t k_base = 0; k_base < k_dim; k_base += 128U) {
        Sbdr128Block blk{};
        blk.k_base = k_base;
        for (std::uint32_t bit = 0; bit < 64U; ++bit) {
          const std::int8_t v0 = wrow[k_base + bit];
          if (v0 > 0)
            blk.pos_lo |= (1ULL << bit);
          else if (v0 < 0)
            blk.neg_lo |= (1ULL << bit);
          const std::int8_t v1 = wrow[k_base + 64U + bit];
          if (v1 > 0)
            blk.pos_hi |= (1ULL << bit);
          else if (v1 < 0)
            blk.neg_hi |= (1ULL << bit);
        }
        if ((blk.pos_lo | blk.pos_hi | blk.neg_lo | blk.neg_hi) != 0ULL) {
          buf.sbdr128_blocks.push_back(blk);
        }
      }
    }
    buf.sbdr128_row_offsets[total_rows] = static_cast<std::uint32_t>(buf.sbdr128_blocks.size());
    kernel_struct_bytes = buf.sbdr128_row_offsets.size() * sizeof(std::uint32_t) +
                          buf.sbdr128_blocks.size() * sizeof(Sbdr128Block);
  } else if (kernel_kind == KernelExecKind::Int8Sparse128) {
    buf.csr_row_offsets.resize(total_rows + 1, 0U);
    buf.csr_cols.reserve(total_weights / 3U);
    buf.csr_vals.reserve(total_weights / 3U);
    for (std::size_t r = 0; r < total_rows; ++r) {
      buf.csr_row_offsets[r] = static_cast<std::uint32_t>(buf.csr_cols.size());
      const std::int8_t* wrow = buf.weights.data() + r * k_dim;
      for (std::uint32_t k = 0; k < k_dim; ++k) {
        if (wrow[k] != 0) {
          buf.csr_cols.push_back(static_cast<std::uint16_t>(k));
          buf.csr_vals.push_back(wrow[k]);
        }
      }
    }
    buf.csr_row_offsets[total_rows] = static_cast<std::uint32_t>(buf.csr_cols.size());
    kernel_struct_bytes = buf.csr_row_offsets.size() * sizeof(std::uint32_t) +
                          buf.csr_cols.size() * sizeof(std::uint16_t) + buf.csr_vals.size();
  }

  buf.actual_working_set_bytes = buf.weights.size() + kernel_struct_bytes + act_bytes + out_bytes;
  return buf;
}

inline std::int64_t dot_dual_rail_word(const std::int8_t* act_base, std::uint64_t pos,
                                       std::uint64_t neg) noexcept {
  std::int64_t acc = 0;
  while (pos != 0ULL) {
    const unsigned bit = static_cast<unsigned>(std::countr_zero(pos));
    acc += static_cast<std::int64_t>(act_base[bit]);
    pos &= (pos - 1ULL);
  }
  while (neg != 0ULL) {
    const unsigned bit = static_cast<unsigned>(std::countr_zero(neg));
    acc -= static_cast<std::int64_t>(act_base[bit]);
    neg &= (neg - 1ULL);
  }
  return acc;
}

void compute_batch_range(const WorkloadBuffers& buf, KernelExecKind kernel_kind,
                         std::uint32_t bank_idx, std::uint32_t b_start, std::uint32_t b_end,
                         std::vector<std::int64_t>& out) noexcept {
  const std::size_t row_base = static_cast<std::size_t>(bank_idx % buf.num_banks) * buf.out_dim;

  if (kernel_kind == KernelExecKind::DualRailDense) {
    for (std::uint32_t b = b_start; b < b_end; ++b) {
      const std::int8_t* act = buf.activations.data() + static_cast<std::size_t>(b) * buf.k_dim;
      std::int64_t* dst = out.data() + static_cast<std::size_t>(b) * buf.out_dim;
      for (std::uint32_t o = 0; o < buf.out_dim; ++o) {
        const DualRailWord* drow = buf.dual_rail.data() + (row_base + o) * buf.words_per_row;
        std::int64_t acc = 0;
        for (std::uint32_t w = 0; w < buf.words_per_row; ++w) {
          acc +=
              dot_dual_rail_word(act + static_cast<std::size_t>(w) * 64U, drow[w].pos, drow[w].neg);
        }
        dst[o] = acc;
      }
    }
    return;
  }

  if (kernel_kind == KernelExecKind::Sbdr128Simd) {
    for (std::uint32_t b = b_start; b < b_end; ++b) {
      const std::int8_t* act = buf.activations.data() + static_cast<std::size_t>(b) * buf.k_dim;
      std::int64_t* dst = out.data() + static_cast<std::size_t>(b) * buf.out_dim;
      for (std::uint32_t o = 0; o < buf.out_dim; ++o) {
        const std::uint32_t begin = buf.sbdr128_row_offsets[row_base + o];
        const std::uint32_t end = buf.sbdr128_row_offsets[row_base + o + 1];
        std::int64_t acc = 0;
        for (std::uint32_t idx = begin; idx < end; ++idx) {
          const Sbdr128Block& blk = buf.sbdr128_blocks[idx];
          acc += dot_dual_rail_word(act + blk.k_base, blk.pos_lo, blk.neg_lo);
          acc += dot_dual_rail_word(act + blk.k_base + 64U, blk.pos_hi, blk.neg_hi);
        }
        dst[o] = acc;
      }
    }
    return;
  }

  if (kernel_kind == KernelExecKind::Int8Sparse128) {
    for (std::uint32_t b = b_start; b < b_end; ++b) {
      const std::int8_t* act = buf.activations.data() + static_cast<std::size_t>(b) * buf.k_dim;
      std::int64_t* dst = out.data() + static_cast<std::size_t>(b) * buf.out_dim;
      for (std::uint32_t o = 0; o < buf.out_dim; ++o) {
        const std::uint32_t begin = buf.csr_row_offsets[row_base + o];
        const std::uint32_t end = buf.csr_row_offsets[row_base + o + 1];
        std::int64_t acc = 0;
        for (std::uint32_t idx = begin; idx < end; ++idx) {
          acc += static_cast<std::int64_t>(act[buf.csr_cols[idx]]) *
                 static_cast<std::int64_t>(buf.csr_vals[idx]);
        }
        dst[o] = acc;
      }
    }
    return;
  }

  // SBDR1T2D: 2D-tiled (8 output rows x 64-element k-word) dual-rail traversal.
  constexpr std::uint32_t kOutTile = 8U;
  for (std::uint32_t b = b_start; b < b_end; ++b) {
    const std::int8_t* act = buf.activations.data() + static_cast<std::size_t>(b) * buf.k_dim;
    std::int64_t* dst = out.data() + static_cast<std::size_t>(b) * buf.out_dim;
    for (std::uint32_t o0 = 0; o0 < buf.out_dim; o0 += kOutTile) {
      const std::uint32_t o_end = std::min(buf.out_dim, o0 + kOutTile);
      std::int64_t tile_acc[kOutTile] = {0, 0, 0, 0, 0, 0, 0, 0};
      for (std::uint32_t w = 0; w < buf.words_per_row; ++w) {
        const std::int8_t* act_w = act + static_cast<std::size_t>(w) * 64U;
        for (std::uint32_t o = o0; o < o_end; ++o) {
          const DualRailWord& dw = buf.dual_rail[(row_base + o) * buf.words_per_row + w];
          tile_acc[o - o0] += dot_dual_rail_word(act_w, dw.pos, dw.neg);
        }
      }
      for (std::uint32_t o = o0; o < o_end; ++o) {
        dst[o] = tile_acc[o - o0];
      }
    }
  }
}

void execute_backend(const WorkloadBuffers& buf, KernelExecKind kernel_kind, std::uint32_t bank_idx,
                     std::uint32_t batch, BackendKind backend, std::uint32_t workers,
                     std::uint32_t tile_batch, std::uint32_t metal_queue_depth,
                     std::uint32_t metal_max_queue_depth, std::vector<std::int64_t>& out,
                     bool& used_metal_gpu, bool& metal_fell_back) {
  out.assign(static_cast<std::size_t>(batch) * buf.out_dim, 0);
  used_metal_gpu = false;
  metal_fell_back = false;

  const std::size_t bank_weight_offset =
      static_cast<std::size_t>(bank_idx % buf.num_banks) * buf.out_dim * buf.k_dim;

  if (backend == BackendKind::MetalGpu) {
    if (t81::bench::h06_mem_04::metal_execute_ternary_matmul(
            buf.weights.data() + bank_weight_offset, buf.activations.data(), out.data(), batch,
            buf.out_dim, buf.k_dim, metal_queue_depth, metal_max_queue_depth)) {
      used_metal_gpu = true;
      return;
    }
    // Deterministic fallback to cpu_simd_mt when Metal is unavailable or queue_depth > threshold.
    metal_fell_back = true;
    backend = BackendKind::CpuSimdMT;
    workers = std::max<std::uint32_t>(1U, workers);
  }

  if (backend == BackendKind::CpuSimd1T || workers <= 1U || batch <= 1U) {
    compute_batch_range(buf, kernel_kind, bank_idx, 0, batch, out);
    return;
  }

  const std::uint32_t step_tile = std::max<std::uint32_t>(1U, std::min(tile_batch, batch));
  for (std::uint32_t tile_start = 0; tile_start < batch; tile_start += step_tile) {
    const std::uint32_t tile_end = std::min(batch, tile_start + step_tile);
    const std::uint32_t tile_len = tile_end - tile_start;
    const std::uint32_t active_w = std::min(workers, tile_len);
    if (active_w <= 1U) {
      compute_batch_range(buf, kernel_kind, bank_idx, tile_start, tile_end, out);
      continue;
    }
    std::vector<std::thread> pool;
    pool.reserve(active_w);
    const std::uint32_t chunk = (tile_len + active_w - 1U) / active_w;
    for (std::uint32_t t = 0; t < active_w; ++t) {
      const std::uint32_t s = tile_start + t * chunk;
      if (s >= tile_end) break;
      const std::uint32_t e = std::min(tile_end, s + chunk);
      pool.emplace_back(
          [&, s, e]() { compute_batch_range(buf, kernel_kind, bank_idx, s, e, out); });
    }
    for (auto& th : pool) th.join();
  }
}

std::string extract_json_string(const std::string& json, const std::string& key,
                                const std::string& fallback) {
  const std::string needle = "\"" + key + "\"";
  const auto kpos = json.find(needle);
  if (kpos == std::string::npos) return fallback;
  const auto colon = json.find(':', kpos + needle.size());
  if (colon == std::string::npos) return fallback;
  const auto q1 = json.find('"', colon + 1);
  if (q1 == std::string::npos) return fallback;
  const auto q2 = json.find('"', q1 + 1);
  if (q2 == std::string::npos) return fallback;
  return json.substr(q1 + 1, q2 - q1 - 1);
}

std::uint64_t extract_json_u64(const std::string& json, const std::string& key,
                               std::uint64_t fallback) {
  const std::string needle = "\"" + key + "\"";
  const auto kpos = json.find(needle);
  if (kpos == std::string::npos) return fallback;
  const auto colon = json.find(':', kpos + needle.size());
  if (colon == std::string::npos) return fallback;
  std::size_t p = colon + 1;
  while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\n')) ++p;
  std::size_t end = p;
  while (end < json.size() && json[end] >= '0' && json[end] <= '9') ++end;
  if (end == p) return fallback;
  try {
    return std::stoull(json.substr(p, end - p));
  } catch (...) {
    return fallback;
  }
}

bool extract_json_bool(const std::string& json, const std::string& key, bool fallback) {
  const std::string needle = "\"" + key + "\"";
  const auto kpos = json.find(needle);
  if (kpos == std::string::npos) return fallback;
  const auto colon = json.find(':', kpos + needle.size());
  if (colon == std::string::npos) return fallback;
  std::size_t p = colon + 1;
  while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\n')) ++p;
  if (json.compare(p, 4, "true") == 0) return true;
  if (json.compare(p, 5, "false") == 0) return false;
  return fallback;
}

double percentile(std::vector<double> vals, double pct) {
  if (vals.empty()) return 0.0;
  std::sort(vals.begin(), vals.end());
  const double idx = (pct / 100.0) * static_cast<double>(vals.size() - 1);
  const auto lo = static_cast<std::size_t>(idx);
  const auto hi = std::min(vals.size() - 1, lo + 1);
  const double frac = idx - static_cast<double>(lo);
  return vals[lo] * (1.0 - frac) + vals[hi] * frac;
}

}  // namespace

int main(int argc, char** argv) {
  std::string manifest_path;
  std::string output_path;
  bool smoke = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--manifest" && i + 1 < argc) {
      manifest_path = argv[++i];
    } else if (arg == "--output" && i + 1 < argc) {
      output_path = argv[++i];
    } else if (arg == "--smoke") {
      smoke = true;
    }
  }

  if (manifest_path.empty() || output_path.empty()) {
    std::cerr
        << "Usage: t81_bench_h06_mem_04 --manifest <path> --output <results.json> [--smoke]\n";
    return 2;
  }

  std::ifstream mf(manifest_path);
  if (!mf) {
    std::cerr << "Error: cannot open manifest: " << manifest_path << "\n";
    return 1;
  }
  std::ostringstream mss;
  mss << mf.rdbuf();
  const std::string manifest_text = mss.str();

  const std::string benchmark_id =
      extract_json_string(manifest_text, "benchmark_id", "h06-mem-04-unknown");
  const std::string git_sha = extract_json_string(manifest_text, "git_sha", "unknown");
  const std::string branch = extract_json_string(manifest_text, "branch", "main");
  const std::string build_flags =
      extract_json_string(manifest_text, "build_flags", "-O3 -march=native -DNDEBUG");

  HardwareProfile hw = HardwareProfile::probe_host();
  const std::string hw_name = extract_json_string(manifest_text, "name", "host");
  const std::string hw_arch =
      extract_json_string(manifest_text, "arch", std::string(arch_name(hw.arch)));
  if (hw_arch == "arm64")
    hw.arch = ArchKind::Arm64;
  else if (hw_arch == "x86_64")
    hw.arch = ArchKind::X86_64;
  hw.has_neon = extract_json_bool(manifest_text, "has_neon", hw.has_neon);
  hw.has_avx2 = extract_json_bool(manifest_text, "has_avx2", hw.has_avx2);
  hw.physical_cores = static_cast<std::uint32_t>(
      extract_json_u64(manifest_text, "physical_cores", hw.physical_cores));
  hw.l2_cache_bytes = extract_json_u64(manifest_text, "l2_cache_bytes", hw.l2_cache_bytes);
  hw.l3_cache_bytes = extract_json_u64(manifest_text, "l3_cache_bytes", hw.l3_cache_bytes);
  const bool manifest_metal = extract_json_bool(manifest_text, "metal_available", false);
  const bool runtime_metal = t81::bench::h06_mem_04::metal_backend_available();
  // Never report or time Metal GPU unless both manifest and compiled runtime support Metal.
  hw.metal_available = manifest_metal && runtime_metal;
  hw.metal_vram_budget_bytes =
      hw.metal_available
          ? extract_json_u64(manifest_text, "metal_vram_budget_bytes", hw.metal_vram_budget_bytes)
          : 0ULL;

  DispatchPolicy pol;
  pol.r0_max_batch = static_cast<std::uint32_t>(extract_json_u64(manifest_text, "r0_max_batch", 4));
  pol.r1_max_batch =
      static_cast<std::uint32_t>(extract_json_u64(manifest_text, "r1_max_batch", 32));
  pol.metal_min_batch =
      static_cast<std::uint32_t>(extract_json_u64(manifest_text, "metal_min_batch", 16));
  pol.metal_min_working_set_bytes =
      extract_json_u64(manifest_text, "metal_min_working_set_bytes", 64ULL * 1024ULL * 1024ULL);
  pol.stream_worker_cap =
      static_cast<std::uint32_t>(extract_json_u64(manifest_text, "stream_worker_cap", 4));

  const std::uint32_t sparsity_pct =
      static_cast<std::uint32_t>(extract_json_u64(manifest_text, "sparsity_pct", 75));
  const std::uint32_t warmup_iters =
      smoke ? 1U : static_cast<std::uint32_t>(extract_json_u64(manifest_text, "warmup_iters", 2));
  const std::uint32_t measure_iters =
      smoke ? 3U : static_cast<std::uint32_t>(extract_json_u64(manifest_text, "measure_iters", 7));

  const std::vector<std::uint32_t> batch_sizes =
      smoke ? std::vector<std::uint32_t>{1, 4, 16, 64}
            : std::vector<std::uint32_t>{1, 4, 8, 16, 32, 64, 128};
  const std::vector<std::string> tiers = {"L2_RESIDENT", "L3_RESIDENT", "OFF_L3_STREAMING",
                                          "OUT_OF_CORE"};
  const std::vector<std::string> kernels = {"DualRail_Dense", "SBDR128_SIMD", "INT8_Sparse128",
                                            "SBDR1T2D"};
  std::vector<std::string> backends = {"auto_dispatch", "cpu_simd_1t", "cpu_simd_mt",
                                       "cpu_stream_mt"};
  if (hw.metal_available) {
    backends.push_back("metal_gpu");
  }

  RegimeSelector selector(hw, pol);

  std::ostringstream out_json;
  out_json << std::fixed << std::setprecision(4);
  out_json << "{\n"
           << "  \"benchmark_id\": \"" << benchmark_id << "\",\n"
           << "  \"provenance\": {\n"
           << "    \"git_sha\": \"" << git_sha << "\",\n"
           << "    \"branch\": \"" << branch << "\",\n"
           << "    \"build_flags\": \"" << build_flags << "\",\n"
           << "    \"ndebug\": "
#ifdef NDEBUG
           << "true\n"
#else
           << "false\n"
#endif
           << "  },\n"
           << "  \"hardware_profile\": {\n"
           << "    \"name\": \"" << hw_name << "\",\n"
           << "    \"arch\": \"" << arch_name(hw.arch) << "\",\n"
           << "    \"has_neon\": " << (hw.has_neon ? "true" : "false") << ",\n"
           << "    \"has_avx2\": " << (hw.has_avx2 ? "true" : "false") << ",\n"
           << "    \"physical_cores\": " << hw.physical_cores << ",\n"
           << "    \"l2_cache_bytes\": " << hw.l2_cache_bytes << ",\n"
           << "    \"l3_cache_bytes\": " << hw.l3_cache_bytes << ",\n"
           << "    \"metal_available\": " << (hw.metal_available ? "true" : "false") << ",\n"
           << "    \"metal_vram_budget_bytes\": " << hw.metal_vram_budget_bytes << "\n"
           << "  },\n"
           << "  \"sweep_axes\": {\n"
           << "    \"batch_sizes\": [";
  for (std::size_t i = 0; i < batch_sizes.size(); ++i) {
    if (i > 0) out_json << ", ";
    out_json << batch_sizes[i];
  }
  out_json << "],\n"
           << "    \"working_set_tiers\": [\"L2_RESIDENT\", \"L3_RESIDENT\", \"OFF_L3_STREAMING\", "
              "\"OUT_OF_CORE\"],\n"
           << "    \"kernels\": [\"DualRail_Dense\", \"SBDR128_SIMD\", \"INT8_Sparse128\", "
              "\"SBDR1T2D\"],\n"
           << "    \"backends\": [";
  for (std::size_t i = 0; i < backends.size(); ++i) {
    if (i > 0) out_json << ", ";
    out_json << "\"" << backends[i] << "\"";
  }
  out_json << "]\n  },\n"
           << "  \"dispatch_policy\": {\n"
           << "    \"r0_max_batch\": " << pol.r0_max_batch << ",\n"
           << "    \"r1_max_batch\": " << pol.r1_max_batch << ",\n"
           << "    \"l2_resident_ratio\": " << pol.l2_resident_ratio << ",\n"
           << "    \"l3_resident_ratio\": " << pol.l3_resident_ratio << ",\n"
           << "    \"metal_min_batch\": " << pol.metal_min_batch << ",\n"
           << "    \"metal_min_working_set_bytes\": " << pol.metal_min_working_set_bytes << ",\n"
           << "    \"hysteresis_gain_ratio\": " << pol.hysteresis_gain_ratio << ",\n"
           << "    \"stream_worker_cap\": " << pol.stream_worker_cap << "\n"
           << "  },\n"
           << "  \"measurement\": {\n"
           << "    \"warmup_iters\": " << warmup_iters << ",\n"
           << "    \"measure_iters\": " << measure_iters << ",\n"
           << "    \"sparsity_pct\": " << sparsity_pct << "\n"
           << "  },\n"
           << "  \"regression_gates\": {\n"
           << "    \"require_exact_determinism\": true,\n"
           << "    \"max_p95_regression_pct\": 5.0,\n"
           << "    \"min_throughput_ratio\": 0.95\n"
           << "  },\n"
           << "  \"results\": [\n";

  bool first_row = true;
  const std::uint32_t out_dim = smoke ? 64U : 128U;
  const std::uint32_t k_dim = smoke ? 256U : 512U;

  for (std::size_t ti = 0; ti < tiers.size(); ++ti) {
    const std::string& tier = tiers[ti];
    const std::uint64_t target_ws = tier_target_bytes(tier, hw, smoke);
    for (std::uint32_t batch : batch_sizes) {
      for (std::size_t ki = 0; ki < kernels.size(); ++ki) {
        const std::string& kernel = kernels[ki];
        const KernelExecKind kernel_kind = parse_kernel_kind(kernel);
        // Same seed per (tier, batch) across all 4 kernels so bit-exactness also holds
        // across DualRail_Dense, SBDR128_SIMD, INT8_Sparse128, and SBDR1T2D.
        const std::uint64_t seed =
            (static_cast<std::uint64_t>(ti + 1) << 32) ^ (static_cast<std::uint64_t>(batch) << 8);
        const WorkloadBuffers buf =
            build_buffers(batch, out_dim, k_dim, sparsity_pct, seed, target_ws, kernel_kind);
        const std::uint64_t actual_ws_bytes = buf.actual_working_set_bytes;

        for (const std::string& backend_str : backends) {
          selector.reset_hysteresis();
          WorkloadQuery q;
          q.batch_size = batch;
          q.working_set_bytes = actual_ws_bytes;
          q.sparsity_pct = sparsity_pct;
          q.is_r2_int8_act = (kernel == "INT8_Sparse128");

          const auto d0 = std::chrono::steady_clock::now();
          const DispatchDecision dec = selector.select(q);
          const auto d1 = std::chrono::steady_clock::now();
          const double dispatch_us = std::chrono::duration<double, std::micro>(d1 - d0).count();

          BackendKind run_backend = dec.backend;
          std::uint32_t run_workers = dec.worker_threads;
          std::uint32_t run_tile = dec.tile_batch_size;
          if (backend_str == "cpu_simd_1t") {
            run_backend = BackendKind::CpuSimd1T;
            run_workers = 1U;
            run_tile = batch;
          } else if (backend_str == "cpu_simd_mt") {
            run_backend = BackendKind::CpuSimdMT;
            run_workers = std::min(hw.physical_cores, batch);
            run_tile = batch;
          } else if (backend_str == "cpu_stream_mt") {
            run_backend = BackendKind::CpuStreamMT;
            run_workers =
                std::min<std::uint32_t>(std::max<std::uint32_t>(1U, pol.stream_worker_cap), batch);
            run_tile = std::max<std::uint32_t>(1U, batch / 2U);
          } else if (backend_str == "metal_gpu") {
            run_backend = BackendKind::MetalGpu;
            run_workers = 1U;
            run_tile = batch;
          }

          std::vector<std::int64_t> out_buf;
          bool used_metal = false;
          bool fell_back = false;
          // Cycle through distinct weight banks across warmup & measurement iterations so
          // the allocated tier working set is actively exercised rather than pinned in L1.
          const std::uint32_t bank_stride = std::max<std::uint32_t>(
              1U, buf.num_banks / std::max<std::uint32_t>(1U, measure_iters));
          for (std::uint32_t w = 0; w < warmup_iters; ++w) {
            const std::uint32_t bank_idx = (w * bank_stride) % buf.num_banks;
            execute_backend(buf, kernel_kind, bank_idx, batch, run_backend, run_workers, run_tile,
                            0U, pol.metal_max_queue_depth, out_buf, used_metal, fell_back);
          }

          std::vector<double> samples_us;
          samples_us.reserve(measure_iters);
          for (std::uint32_t m = 0; m < measure_iters; ++m) {
            const std::uint32_t bank_idx = ((m + 1U) * bank_stride) % buf.num_banks;
            const auto t0 = std::chrono::steady_clock::now();
            execute_backend(buf, kernel_kind, bank_idx, batch, run_backend, run_workers, run_tile,
                            0U, pol.metal_max_queue_depth, out_buf, used_metal, fell_back);
            const auto t1 = std::chrono::steady_clock::now();
            if (backend_str == "metal_gpu" && (!used_metal || fell_back)) {
              std::cerr << "ERROR: metal_gpu row fell back to CPU; refusing to report CPU "
                           "timings as GPU\n";
              return 1;
            }
            samples_us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
          }

          const double p50 = percentile(samples_us, 50.0);
          const double p95 = percentile(samples_us, 95.0);
          const double p99 = percentile(samples_us, 99.0);
          const double total_ops = 2.0 * static_cast<double>(batch) * static_cast<double>(out_dim) *
                                   static_cast<double>(k_dim);
          const double gops = (p50 > 0.0) ? (total_ops / (p50 * 1000.0)) : 0.0;
          const double bytes_moved =
              static_cast<double>(static_cast<std::size_t>(out_dim) * k_dim +
                                  buf.activations.size() + out_buf.size() * sizeof(std::int64_t));
          const double eff_gbps = (p50 > 0.0) ? (bytes_moved / (p50 * 1000.0)) : 0.0;
          const std::string det_hash = hex64(fnv1a_64(out_buf));

          if (!first_row) out_json << ",\n";
          first_row = false;
          out_json << "    {"
                   << "\"tier\": \"" << tier << "\", "
                   << "\"batch_size\": " << batch << ", "
                   << "\"kernel\": \"" << kernel << "\", "
                   << "\"backend\": \"" << backend_str << "\", "
                   << "\"regime_selected\": \"" << regime_name(dec.regime) << "\", "
                   << "\"working_set_bytes\": " << actual_ws_bytes << ", "
                   << "\"p50_us\": " << p50 << ", "
                   << "\"p95_us\": " << p95 << ", "
                   << "\"p99_us\": " << p99 << ", "
                   << "\"gops_per_sec\": " << gops << ", "
                   << "\"effective_gb_per_sec\": " << eff_gbps << ", "
                   << "\"dispatch_overhead_us\": " << dispatch_us << ", "
                   << "\"determinism_hash\": \"" << det_hash << "\""
                   << "}";
        }
      }
    }
  }

  out_json << "\n  ]\n}\n";

  std::ofstream of(output_path);
  if (!of) {
    std::cerr << "Error: cannot write output JSON: " << output_path << "\n";
    return 1;
  }
  of << out_json.str();
  of.close();

  // Verify live Metal GPU + queue_depth > 2 deterministic fallback when Metal is compiled in.
  if (t81::bench::h06_mem_04::metal_backend_available()) {
    const WorkloadBuffers probe_buf = build_buffers(
        16, 64, 256, 75, 0xCAFEBABEULL, 256ULL * 1024ULL, KernelExecKind::DualRailDense);
    std::vector<std::int64_t> ref_out;
    std::vector<std::int64_t> gpu_out;
    std::vector<std::int64_t> fb_out;
    bool u_ref = false, fb_ref = false;
    bool u_gpu = false, fb_gpu = false;
    bool u_fb = false, fb_fb = false;
    execute_backend(probe_buf, KernelExecKind::DualRailDense, 0, 16, BackendKind::CpuSimd1T, 1, 16,
                    0, pol.metal_max_queue_depth, ref_out, u_ref, fb_ref);
    execute_backend(probe_buf, KernelExecKind::DualRailDense, 0, 16, BackendKind::MetalGpu, 4, 16,
                    0, pol.metal_max_queue_depth, gpu_out, u_gpu, fb_gpu);
    execute_backend(probe_buf, KernelExecKind::DualRailDense, 0, 16, BackendKind::MetalGpu, 4, 16,
                    pol.metal_max_queue_depth + 1, pol.metal_max_queue_depth, fb_out, u_fb, fb_fb);
    if (!u_gpu || fb_gpu || fnv1a_64(gpu_out) != fnv1a_64(ref_out)) {
      std::cerr << "ERROR: live Metal GPU probe failed bit-exactness\n";
      return 1;
    }
    if (u_fb || !fb_fb || fnv1a_64(fb_out) != fnv1a_64(ref_out)) {
      std::cerr << "ERROR: Metal queue_depth > 2 fallback failed bit-exactness\n";
      return 1;
    }
    std::cout << "METAL_GPU_VERIFIED=1 METAL_QUEUE_FALLBACK_VERIFIED=1 hash="
              << hex64(fnv1a_64(ref_out)) << "\n";
  }

  std::cout << "Wrote H06-MEM-04 results to " << output_path << "\n";
  return 0;
}
