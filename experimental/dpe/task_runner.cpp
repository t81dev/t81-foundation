// experimental/dpe/task_runner.cpp
//
// RFC-DPE-0002 §5 / [DPE-02-05]: DpeTaskRunner implementation.
// RFC-DPE-0003 [DPE-03-05]: T81Float determinism in DPE tasks.
//
// Float handle serialization contract:
//   - DeltaRecord words tagged ValueTag::FloatHandle (tag byte = 3) contain
//     the IEEE 754 double representation of the float value (not the transient
//     pool handle index).  This makes delta pages self-contained and ensures
//     identical EpochHash across runs regardless of handle allocation order.
//   - On snapshot load, FloatHandle-tagged words are interned into the VM's
//     float pool via intern_float(), producing a fresh handle that is then
//     stored with set_memory_word_tagged().

#include "task_runner.hpp"
#include "t81/vm/vm.hpp"
#include "t81/vm/state.hpp"  // ValueTag

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
  //
  // FloatHandle-tagged words: the 8 bytes contain an IEEE 754 double (not a
  // pool handle index).  We intern the value into the VM's float pool to get a
  // fresh handle, then write the handle with the FloatHandle tag.
  for (const auto& [word_start, page] : snapshot.pages) {
    for (std::size_t w = 0; w < kWordsPerPage; ++w) {
      const std::size_t byte_off = w * sizeof(std::int64_t);
      if (byte_off + sizeof(std::int64_t) > kDpePageSize) break;

      const std::size_t dest_word = static_cast<std::size_t>(word_start) + w;
      const auto tag = static_cast<t81::vm::ValueTag>(page.word_tags[w]);

      if (tag == t81::vm::ValueTag::FloatHandle) {
        double fval = 0.0;
        std::memcpy(&fval, page.bytes.data() + byte_off, sizeof(fval));
        const std::int64_t handle = vm->intern_float(fval);
        vm->set_memory_word_tagged(dest_word, handle, t81::vm::ValueTag::FloatHandle);
      } else {
        std::int64_t word_val = 0;
        std::memcpy(&word_val, page.bytes.data() + byte_off, sizeof(word_val));
        vm->set_memory_word(dest_word, word_val);
      }
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

      // Pack changed page words into the byte array.
      // FloatHandle words: serialize the actual double value (IEEE 754 LE) so
      // the delta is independent of transient pool handle indices, satisfying
      // [DPE-03-05].  All other words are packed as little-endian int64_t.
      for (std::size_t w = page_word_start; w < page_word_end; ++w) {
        const std::size_t slot    = w - page_word_start;
        const std::size_t byte_off = slot * sizeof(std::int64_t);
        if (byte_off + sizeof(std::int64_t) > kDpePageSize) break;

        const auto tag = (w < post.memory_tags.size())
                             ? post.memory_tags[w]
                             : t81::vm::ValueTag::Int;
        rec.word_tags[slot] = static_cast<std::uint8_t>(tag);

        if (tag == t81::vm::ValueTag::FloatHandle) {
          // Resolve handle to float value; write canonical double bytes.
          const std::int64_t handle = post.memory[w];
          double fval = 0.0;
          if (handle > 0) {
            const std::size_t idx = static_cast<std::size_t>(handle - 1);
            if (idx < post.floats.size()) fval = post.floats[idx];
          }
          std::memcpy(rec.value.data() + byte_off, &fval, sizeof(fval));
        } else {
          const std::int64_t word = post.memory[w];
          std::memcpy(rec.value.data() + byte_off, &word, sizeof(word));
        }
      }

      result.delta_records.push_back(std::move(rec));
    }
  }

  return result;
}

}  // namespace t81::dpe
