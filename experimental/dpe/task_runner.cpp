// experimental/dpe/task_runner.cpp
//
// RFC-DPE-0002 §5 / [DPE-02-05]: DpeTaskRunner implementation.

#include "task_runner.hpp"
#include "t81/vm/vm.hpp"

#include <algorithm>
#include <cstring>

namespace t81::dpe {

// Words per DPE page in the hosted VM simulation.
// kDpePageSize bytes / 8 bytes-per-int64_t = 512 words/page.
static constexpr std::size_t kWordsPerPage = kDpePageSize / sizeof(std::int64_t);

DpeTaskResult DpeTaskRunner::run_direct(
    const TaskDescriptor& task,
    const t81::tisc::Program& program,
    const DpeTaskInputSnapshot& snapshot) noexcept {

  DpeTaskResult result;

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);

  // ── Load predecessor input snapshot (RFC-DPE-0004 §3.2) ──────────────────
  // Each page in the snapshot is unpacked (little-endian int64_t words) and
  // written into VM flat memory at the corresponding word positions.
  for (const auto& [word_start, page_bytes] : snapshot.pages) {
    for (std::size_t w = 0; w < kWordsPerPage; ++w) {
      const std::size_t byte_off = w * sizeof(std::int64_t);
      if (byte_off + sizeof(std::int64_t) > kDpePageSize) break;
      std::int64_t word_val = 0;
      std::memcpy(&word_val, page_bytes.data() + byte_off, sizeof(word_val));
      vm->set_memory_word(static_cast<std::size_t>(word_start) + w, word_val);
    }
  }

  // ── Snapshot output regions before execution ──────────────────────────────
  // base_tva is treated as a flat word index into State::memory for the
  // hosted simulation.  We snapshot only the pages declared in output_regions
  // so we can diff them after execution.

  const auto& pre = vm->state();

  struct RegionSnapshot {
    std::size_t              start_word{0};  // word index in State::memory
    std::vector<std::int64_t> words{};       // copy of pre-execution words
  };

  std::vector<RegionSnapshot> snapshots;
  snapshots.reserve(task.output_regions.size());

  for (const auto& reg : task.output_regions) {
    const std::size_t start = static_cast<std::size_t>(reg.base_tva);
    const std::size_t count = static_cast<std::size_t>(reg.page_count) * kWordsPerPage;

    RegionSnapshot snap;
    snap.start_word = start;
    if (start < pre.memory.size()) {
      const std::size_t end = std::min(start + count, pre.memory.size());
      snap.words.assign(pre.memory.begin() + static_cast<std::ptrdiff_t>(start),
                        pre.memory.begin() + static_cast<std::ptrdiff_t>(end));
    }
    snapshots.push_back(std::move(snap));
  }

  // ── Execute ───────────────────────────────────────────────────────────────
  (void)vm->run_to_halt();

  const auto& post = vm->state();
  result.halted = post.halted;

  // Copy final register file from thread context 0.
  if (!post.contexts.empty()) {
    result.final_registers = post.contexts[0].registers;
  }

  // ── Emit DeltaRecords for pages whose content changed ─────────────────────
  // For each output region, walk page by page.  If any word in the page
  // differs from the pre-execution snapshot, emit one DeltaRecord whose
  // 4 096-byte value array carries the post-execution page (words packed
  // little-endian).

  const TaskId tid = compute_task_id(task);

  for (std::size_t ri = 0; ri < task.output_regions.size(); ++ri) {
    const OutputRegion& reg  = task.output_regions[ri];
    const RegionSnapshot& sn = snapshots[ri];

    for (uint32_t p = 0; p < reg.page_count; ++p) {
      const std::size_t page_word_start = sn.start_word + p * kWordsPerPage;
      if (page_word_start >= post.memory.size()) break;

      const std::size_t page_word_end =
          std::min(page_word_start + kWordsPerPage, post.memory.size());

      // Detect whether any word in this page changed.
      bool changed = false;
      for (std::size_t w = page_word_start; w < page_word_end; ++w) {
        const std::size_t snap_idx = w - sn.start_word;
        const std::int64_t before =
            snap_idx < sn.words.size() ? sn.words[snap_idx] : 0;
        if (post.memory[w] != before) {
          changed = true;
          break;
        }
      }
      if (!changed) continue;

      DeltaRecord rec;
      rec.task_id = tid;
      rec.tva     = reg.base_tva + static_cast<uint64_t>(p) * kDpePageSize;

      // Pack changed page words into the byte array (little-endian int64_t).
      for (std::size_t w = page_word_start; w < page_word_end; ++w) {
        const std::size_t byte_off = (w - page_word_start) * sizeof(std::int64_t);
        if (byte_off + sizeof(std::int64_t) > kDpePageSize) break;
        const std::int64_t word = post.memory[w];
        std::memcpy(rec.value.data() + byte_off, &word, sizeof(word));
      }

      result.delta_records.push_back(std::move(rec));
    }
  }

  return result;
}

}  // namespace t81::dpe
