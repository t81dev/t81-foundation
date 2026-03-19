#include <benchmark/benchmark.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "t81/dpe/epoch_commit.hpp"
#include "t81/dpe/task_graph.hpp"
#include "t81/dpe/task_runner.hpp"
#include "t81/dpe/thread_pool.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

namespace {

using t81::dpe::DeltaRecord;
using t81::dpe::EpochGraph;
using t81::dpe::OutputRegion;
using t81::dpe::TaskDeltaSet;
using t81::dpe::TaskDescriptor;
using t81::dpe::TaskId;
using t81::dpe::DpeTaskResult;

constexpr std::int32_t kHeapWord = 259;
constexpr std::uint64_t kPageA = 512;
constexpr std::uint64_t kPageB = 768;

t81::tisc::Program make_store_program(std::int32_t value, std::int32_t word);
t81::tisc::Program make_load_add_store_program(std::int32_t addend,
                                               std::int32_t src_word,
                                               std::int32_t dst_word);

TaskDescriptor make_task(std::uint64_t epoch_id, std::uint64_t task_seq) {
  TaskDescriptor task;
  task.epoch_id = epoch_id;
  task.task_seq = task_seq;
  task.output_regions.push_back(OutputRegion{
      .base_tva = static_cast<std::uint64_t>(0x1000 + task_seq * t81::dpe::kDpePageSize),
      .page_count = 1,
      .exclusive = false,
  });
  return task;
}

EpochGraph make_independent_epoch(std::size_t task_count) {
  EpochGraph epoch;
  epoch.epoch_id = 81 + static_cast<std::uint64_t>(task_count);
  epoch.tasks.reserve(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    epoch.tasks.push_back(make_task(epoch.epoch_id, static_cast<std::uint64_t>(i)));
  }
  return epoch;
}

EpochGraph make_linear_epoch(std::size_t task_count) {
  EpochGraph epoch;
  epoch.epoch_id = 243 + static_cast<std::uint64_t>(task_count);
  epoch.tasks.reserve(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    auto task = make_task(epoch.epoch_id, static_cast<std::uint64_t>(i));
    if (i > 0) {
      task.dep_task_ids.push_back(t81::dpe::program_identity(epoch.tasks.back()));
    }
    epoch.tasks.push_back(std::move(task));
  }
  return epoch;
}

EpochGraph make_fanout_epoch(std::size_t width) {
  EpochGraph epoch;
  epoch.epoch_id = 729 + static_cast<std::uint64_t>(width);

  auto root = make_task(epoch.epoch_id, 0);
  const TaskId root_id = t81::dpe::program_identity(root);
  epoch.tasks.push_back(root);

  for (std::size_t i = 0; i < width; ++i) {
    auto task = make_task(epoch.epoch_id, static_cast<std::uint64_t>(i + 1));
    task.dep_task_ids.push_back(root_id);
    epoch.tasks.push_back(std::move(task));
  }
  return epoch;
}

EpochGraph make_chain_epoch_for_programs(std::size_t task_count, std::int32_t base_word) {
  EpochGraph epoch;
  epoch.epoch_id = 5000 + static_cast<std::uint64_t>(task_count);
  epoch.tasks.reserve(task_count);

  for (std::size_t i = 0; i < task_count; ++i) {
    TaskDescriptor task;
    task.epoch_id = epoch.epoch_id;
    task.task_seq = static_cast<std::uint64_t>(i);
    const auto word = static_cast<std::int32_t>(base_word + static_cast<std::int32_t>(i * 512));
    task.output_regions.push_back(OutputRegion{static_cast<std::uint64_t>(word), 1, true});
    if (!epoch.tasks.empty()) {
      task.dep_task_ids.push_back(t81::dpe::program_identity(epoch.tasks.back()));
    }
    epoch.tasks.push_back(std::move(task));
  }

  return epoch;
}

std::vector<t81::tisc::Program> make_chain_programs(std::size_t task_count,
                                                    std::int32_t base_word,
                                                    std::int32_t seed_value) {
  std::vector<t81::tisc::Program> programs;
  programs.reserve(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    const auto dst_word = static_cast<std::int32_t>(base_word + static_cast<std::int32_t>(i * 512));
    if (i == 0) {
      programs.push_back(make_store_program(seed_value, dst_word));
      continue;
    }
    const auto src_word =
        static_cast<std::int32_t>(base_word + static_cast<std::int32_t>((i - 1) * 512));
    programs.push_back(make_load_add_store_program(1, src_word, dst_word));
  }
  return programs;
}

EpochGraph make_fanout_epoch_for_programs(std::size_t width, std::int32_t base_word) {
  EpochGraph epoch;
  epoch.epoch_id = 6000 + static_cast<std::uint64_t>(width);

  TaskDescriptor root;
  root.epoch_id = epoch.epoch_id;
  root.task_seq = 0;
  root.output_regions.push_back(OutputRegion{static_cast<std::uint64_t>(base_word), 1, true});
  const TaskId root_id = t81::dpe::program_identity(root);
  epoch.tasks.push_back(std::move(root));

  for (std::size_t i = 0; i < width; ++i) {
    TaskDescriptor task;
    task.epoch_id = epoch.epoch_id;
    task.task_seq = static_cast<std::uint64_t>(i + 1);
    const auto dst_word =
        static_cast<std::int32_t>(base_word + static_cast<std::int32_t>((i + 1) * 512));
    task.output_regions.push_back(OutputRegion{static_cast<std::uint64_t>(dst_word), 1, true});
    task.dep_task_ids.push_back(root_id);
    epoch.tasks.push_back(std::move(task));
  }

  return epoch;
}

std::vector<t81::tisc::Program> make_fanout_programs(std::size_t width,
                                                     std::int32_t base_word,
                                                     std::int32_t seed_value) {
  std::vector<t81::tisc::Program> programs;
  programs.reserve(width + 1);
  programs.push_back(make_store_program(seed_value, base_word));
  for (std::size_t i = 0; i < width; ++i) {
    const auto dst_word =
        static_cast<std::int32_t>(base_word + static_cast<std::int32_t>((i + 1) * 512));
    programs.push_back(make_load_add_store_program(static_cast<std::int32_t>(i + 1),
                                                   base_word,
                                                   dst_word));
  }
  return programs;
}

std::vector<TaskDeltaSet> make_delta_sets(const EpochGraph& epoch) {
  std::vector<TaskDeltaSet> delta_sets;
  delta_sets.reserve(epoch.tasks.size());

  for (const auto& task : epoch.tasks) {
    TaskDeltaSet set;
    set.id = t81::dpe::compute_task_id(task);

    DeltaRecord record;
    record.task_id = set.id;
    record.tva = task.output_regions.front().base_tva;
    record.value.fill(std::byte{0});
    record.word_tags.fill(0);
    record.value[0] = static_cast<std::byte>(task.task_seq & 0xFFu);

    set.records.push_back(record);
    delta_sets.push_back(std::move(set));
  }

  return delta_sets;
}

std::vector<TaskDeltaSet> make_delta_sets_with_pages(const EpochGraph& epoch,
                                                     std::size_t pages_per_task) {
  std::vector<TaskDeltaSet> delta_sets;
  delta_sets.reserve(epoch.tasks.size());

  for (const auto& task : epoch.tasks) {
    TaskDeltaSet set;
    set.id = t81::dpe::compute_task_id(task);

    for (std::size_t page_index = 0; page_index < pages_per_task; ++page_index) {
      DeltaRecord record;
      record.task_id = set.id;
      record.tva = task.output_regions.front().base_tva +
                   static_cast<std::uint64_t>(page_index * t81::dpe::kDpePageSize);
      record.value.fill(std::byte{0});
      record.word_tags.fill(0);
      record.value[0] = static_cast<std::byte>((task.task_seq + page_index) & 0xFFu);
      set.records.push_back(record);
    }

    delta_sets.push_back(std::move(set));
  }

  return delta_sets;
}

t81::tisc::Program make_arith_program() {
  t81::tisc::Program program;
  program.insns = {
      {t81::tisc::Opcode::LoadImm, 1, 10},
      {t81::tisc::Opcode::LoadImm, 2, 20},
      {t81::tisc::Opcode::Add, 3, 1, 2},
      {t81::tisc::Opcode::Halt},
  };
  return program;
}

t81::tisc::Program make_store_program(std::int32_t value, std::int32_t word) {
  t81::tisc::Program program;
  program.insns = {
      {t81::tisc::Opcode::LoadImm, 1, value},
      {t81::tisc::Opcode::Store, word, 1},
      {t81::tisc::Opcode::Halt},
  };
  return program;
}

t81::tisc::Program make_load_add_store_program(std::int32_t addend,
                                               std::int32_t src_word,
                                               std::int32_t dst_word) {
  t81::tisc::Program program;
  program.insns = {
      {t81::tisc::Opcode::Load, 2, src_word},
      {t81::tisc::Opcode::LoadImm, 3, addend},
      {t81::tisc::Opcode::Add, 4, 2, 3},
      {t81::tisc::Opcode::Store, dst_word, 4},
      {t81::tisc::Opcode::Halt},
  };
  return program;
}

DpeTaskResult run_task_direct(const TaskDescriptor& task,
                              const t81::tisc::Program& program) {
  t81::dpe::DpeTaskRunner runner;
  return runner.run_direct(task, program);
}

DpeTaskResult run_task_with_optional_snapshot(const TaskDescriptor& task,
                                              const t81::tisc::Program& program,
                                              const t81::dpe::DpeTaskInputSnapshot& snapshot = {}) {
  t81::dpe::DpeTaskRunner runner;
  return runner.run_direct(task, program, snapshot);
}

t81::dpe::DpeTaskInputSnapshot snapshot_from_result(const DpeTaskResult& result) {
  t81::dpe::DpeTaskInputSnapshot snapshot;
  for (const auto& rec : result.delta_records) {
    snapshot.pages.emplace(rec.tva, t81::dpe::DpePageSnapshot{rec.value, rec.word_tags});
  }
  return snapshot;
}

EpochGraph make_epoch_for_store_programs(std::size_t task_count, std::int32_t base_word) {
  EpochGraph epoch;
  epoch.epoch_id = 4000 + static_cast<std::uint64_t>(task_count);
  epoch.tasks.reserve(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    TaskDescriptor task;
    task.epoch_id = epoch.epoch_id;
    task.task_seq = static_cast<std::uint64_t>(i);
    const auto word = static_cast<std::int32_t>(base_word + static_cast<std::int32_t>(i * 512));
    task.output_regions.push_back(OutputRegion{
        static_cast<std::uint64_t>(word), 1, true});
    epoch.tasks.push_back(std::move(task));
  }
  return epoch;
}

std::vector<t81::tisc::Program> make_store_programs(std::size_t task_count,
                                                    std::int32_t base_word,
                                                    std::int32_t value_base) {
  std::vector<t81::tisc::Program> programs;
  programs.reserve(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    const auto word = static_cast<std::int32_t>(base_word + static_cast<std::int32_t>(i * 512));
    programs.push_back(
        make_store_program(static_cast<std::int32_t>(value_base + static_cast<std::int32_t>(i)),
                           word));
  }
  return programs;
}

std::vector<TaskDeltaSet> run_epoch_sequential(const EpochGraph& epoch,
                                               const std::vector<t81::tisc::Program>& programs) {
  std::vector<TaskDeltaSet> delta_sets;
  delta_sets.reserve(epoch.tasks.size());

  for (std::size_t i = 0; i < epoch.tasks.size(); ++i) {
    const auto result = run_task_direct(epoch.tasks[i], programs[i]);
    TaskDeltaSet ds;
    ds.id = t81::dpe::compute_task_id(epoch.tasks[i]);
    ds.faulted = !result.halted;
    ds.records = result.delta_records;
    delta_sets.push_back(std::move(ds));
  }

  return delta_sets;
}

std::vector<TaskDeltaSet> run_epoch_pooled(const EpochGraph& epoch,
                                           const std::vector<t81::tisc::Program>& programs,
                                           std::size_t worker_count) {
  std::vector<DpeTaskResult> results(epoch.tasks.size());
  t81::dpe::DpeThreadPool pool(worker_count);
  t81::dpe::DpeTaskRunner runner;

  for (std::size_t i = 0; i < epoch.tasks.size(); ++i) {
    const std::size_t idx = i;
    const bool submitted = pool.submit([&runner, &epoch, &programs, &results, idx]() {
      results[idx] = runner.run_direct(epoch.tasks[idx], programs[idx]);
    });
    benchmark::DoNotOptimize(static_cast<int>(submitted));
  }
  pool.wait_idle();

  std::vector<TaskDeltaSet> delta_sets;
  delta_sets.reserve(epoch.tasks.size());
  for (std::size_t i = 0; i < epoch.tasks.size(); ++i) {
    TaskDeltaSet ds;
    ds.id = t81::dpe::compute_task_id(epoch.tasks[i]);
    ds.faulted = !results[i].halted;
    ds.records = results[i].delta_records;
    delta_sets.push_back(std::move(ds));
  }
  return delta_sets;
}

DpeTaskResult run_task_for_epoch(const TaskDescriptor& task,
                                 const t81::tisc::Program& program,
                                 const t81::dpe::DpeTaskInputSnapshot& snapshot) {
  t81::dpe::DpeTaskRunner runner;
  return runner.run_direct(task, program, snapshot);
}

t81::dpe::DpeTaskInputSnapshot snapshot_from_dependencies(
    const TaskDescriptor& task,
    const std::map<TaskId, DpeTaskResult>& completed_results) {
  t81::dpe::DpeTaskInputSnapshot snapshot;
  for (const auto& dep_id : task.dep_task_ids) {
    const auto found = completed_results.find(dep_id);
    if (found == completed_results.end()) {
      continue;
    }
    for (const auto& rec : found->second.delta_records) {
      snapshot.pages[rec.tva] = t81::dpe::DpePageSnapshot{rec.value, rec.word_tags};
    }
  }
  return snapshot;
}

std::vector<TaskDeltaSet> run_epoch_by_levels(const EpochGraph& epoch,
                                              const std::vector<t81::tisc::Program>& programs,
                                              std::size_t worker_count) {
  const auto levels = t81::dpe::topological_levels_epoch(epoch);
  std::map<TaskId, DpeTaskResult> completed_results;
  std::vector<TaskDeltaSet> delta_sets;
  delta_sets.reserve(epoch.tasks.size());

  for (const auto& level : levels) {
    std::vector<DpeTaskResult> level_results(level.size());
    std::vector<t81::dpe::DpeTaskInputSnapshot> snapshots(level.size());
    for (std::size_t slot = 0; slot < level.size(); ++slot) {
      snapshots[slot] = snapshot_from_dependencies(epoch.tasks[level[slot]], completed_results);
    }

    if (worker_count <= 1 || level.size() <= 1) {
      for (std::size_t slot = 0; slot < level.size(); ++slot) {
        const auto task_index = level[slot];
        level_results[slot] =
            run_task_for_epoch(epoch.tasks[task_index], programs[task_index], snapshots[slot]);
      }
    } else {
      t81::dpe::DpeThreadPool pool(worker_count);
      for (std::size_t slot = 0; slot < level.size(); ++slot) {
        const auto task_index = level[slot];
        const bool submitted = pool.submit([&epoch, &programs, &level_results, &snapshots, slot,
                                            task_index]() {
          level_results[slot] =
              run_task_for_epoch(epoch.tasks[task_index], programs[task_index], snapshots[slot]);
        });
        benchmark::DoNotOptimize(static_cast<int>(submitted));
      }
      pool.wait_idle();
    }

    for (std::size_t slot = 0; slot < level.size(); ++slot) {
      const auto task_index = level[slot];
      const auto program_id = t81::dpe::program_identity(epoch.tasks[task_index]);
      completed_results[program_id] = level_results[slot];

      TaskDeltaSet ds;
      ds.id = t81::dpe::compute_task_id(epoch.tasks[task_index]);
      ds.faulted = !level_results[slot].halted;
      ds.records = level_results[slot].delta_records;
      delta_sets.push_back(std::move(ds));
    }
  }

  return delta_sets;
}

static void BM_DPE_AcceptEpoch_Independent(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const EpochGraph epoch = make_independent_epoch(task_count);

  for (auto _ : state) {
    auto result = t81::dpe::accept_epoch(epoch);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
}
BENCHMARK(BM_DPE_AcceptEpoch_Independent)->Arg(8)->Arg(64)->Arg(256);

static void BM_DPE_TopologicalLevels_Fanout(benchmark::State& state) {
  const std::size_t width = static_cast<std::size_t>(state.range(0));
  const EpochGraph epoch = make_fanout_epoch(width);

  for (auto _ : state) {
    auto levels = t81::dpe::topological_levels_epoch(epoch);
    benchmark::DoNotOptimize(levels);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(epoch.tasks.size()));
  state.counters["tasks"] = static_cast<double>(epoch.tasks.size());
  state.counters["levels"] = 2.0;
}
BENCHMARK(BM_DPE_TopologicalLevels_Fanout)->Arg(8)->Arg(64)->Arg(256);

static void BM_DPE_CommitEpoch_Linear(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const EpochGraph epoch = make_linear_epoch(task_count);
  const auto delta_sets = make_delta_sets(epoch);

  for (auto _ : state) {
    auto result = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["pages"] = static_cast<double>(task_count);
}
BENCHMARK(BM_DPE_CommitEpoch_Linear)->Arg(8)->Arg(64)->Arg(256);

static void BM_DPE_TaskRunner_RunDirect_NoOutputs(benchmark::State& state) {
  const auto program = make_arith_program();
  t81::dpe::DpeTaskRunner runner;
  TaskDescriptor task;
  task.epoch_id = 2000;
  task.task_seq = 0;

  for (auto _ : state) {
    auto result = runner.run_direct(task, program);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(program.insns.size()));
  state.counters["ops_per_run"] = static_cast<double>(program.insns.size());
  state.counters["delta_pages"] = 0.0;
}
BENCHMARK(BM_DPE_TaskRunner_RunDirect_NoOutputs)->Repetitions(3);

static void BM_DPE_TaskRunner_RunDirect_WithOutputRegion(benchmark::State& state) {
  const std::int32_t value = static_cast<std::int32_t>(state.range(0));
  const auto program = make_store_program(value, kHeapWord);
  t81::dpe::DpeTaskRunner runner;
  TaskDescriptor task;
  task.epoch_id = 2001;
  task.task_seq = 0;
  task.output_regions.push_back(OutputRegion{static_cast<std::uint64_t>(kHeapWord), 1, false});

  for (auto _ : state) {
    auto result = runner.run_direct(task, program);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(program.insns.size()));
  state.counters["ops_per_run"] = static_cast<double>(program.insns.size());
  state.counters["delta_pages"] = 1.0;
}
BENCHMARK(BM_DPE_TaskRunner_RunDirect_WithOutputRegion)->Arg(7)->Arg(81)->Repetitions(3);

static void BM_DPE_ThreadPool_SubmitWait_Independent(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const std::size_t worker_count = static_cast<std::size_t>(state.range(1));

  EpochGraph epoch = make_independent_epoch(task_count);
  std::vector<t81::tisc::Program> programs;
  programs.reserve(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    const auto word = static_cast<std::int32_t>(kHeapWord + static_cast<std::int32_t>(i * 512));
    programs.push_back(make_store_program(static_cast<std::int32_t>(i + 1), word));
    epoch.tasks[i].output_regions.clear();
    epoch.tasks[i].output_regions.push_back(OutputRegion{
        static_cast<std::uint64_t>(word), 1, true});
  }

  for (auto _ : state) {
    t81::dpe::DpeThreadPool pool(worker_count);
    t81::dpe::DpeTaskRunner runner;
    std::vector<DpeTaskResult> results(task_count);

    for (std::size_t i = 0; i < task_count; ++i) {
      const std::size_t idx = i;
      bool submitted = pool.submit([&runner, &epoch, &programs, &results, idx]() {
        results[idx] = runner.run_direct(epoch.tasks[idx], programs[idx]);
      });
      benchmark::DoNotOptimize(static_cast<int>(submitted));
    }

    pool.wait_idle();
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["workers"] = static_cast<double>(worker_count);
}
BENCHMARK(BM_DPE_ThreadPool_SubmitWait_Independent)->Args({4, 1})->Args({4, 2})->Args({16, 2})->Args({16, 4});

static void BM_DPE_CommitEpoch_PageExpansion(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const std::size_t pages_per_task = static_cast<std::size_t>(state.range(1));

  EpochGraph epoch = make_independent_epoch(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    epoch.tasks[i].output_regions.clear();
    epoch.tasks[i].output_regions.push_back(OutputRegion{
        static_cast<std::uint64_t>(0x4000 + i * pages_per_task * t81::dpe::kDpePageSize),
        static_cast<std::uint32_t>(pages_per_task),
        false});
  }
  const auto delta_sets = make_delta_sets_with_pages(epoch, pages_per_task);

  for (auto _ : state) {
    auto result = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() *
                          static_cast<std::int64_t>(task_count * pages_per_task));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["pages_per_task"] = static_cast<double>(pages_per_task);
}
BENCHMARK(BM_DPE_CommitEpoch_PageExpansion)->Args({8, 1})->Args({8, 4})->Args({32, 1})->Args({32, 4});

static void BM_DPE_TaskRunner_PredecessorSnapshotLoad(benchmark::State& state) {
  const auto producer_program = make_store_program(42, static_cast<std::int32_t>(kPageA));
  const auto consumer_program =
      make_load_add_store_program(1, static_cast<std::int32_t>(kPageA), static_cast<std::int32_t>(kPageB));

  TaskDescriptor producer;
  producer.epoch_id = 3000;
  producer.task_seq = 0;
  producer.output_regions.push_back(OutputRegion{kPageA, 1, true});

  TaskDescriptor consumer;
  consumer.epoch_id = 3000;
  consumer.task_seq = 1;
  consumer.output_regions.push_back(OutputRegion{kPageB, 1, true});
  consumer.dep_task_ids.push_back(t81::dpe::program_identity(producer));

  const auto producer_result = run_task_with_optional_snapshot(producer, producer_program);
  const auto snapshot = snapshot_from_result(producer_result);

  for (auto _ : state) {
    auto result = run_task_with_optional_snapshot(consumer, consumer_program, snapshot);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * 5);
  state.counters["snapshot_pages"] = 1.0;
  state.counters["ops_per_run"] = 5.0;
}
BENCHMARK(BM_DPE_TaskRunner_PredecessorSnapshotLoad)->Repetitions(3);

static void BM_DPE_EpochHash_SequentialVsPooled(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const std::size_t worker_count = static_cast<std::size_t>(state.range(1));

  EpochGraph epoch = make_independent_epoch(task_count);
  std::vector<t81::tisc::Program> programs;
  programs.reserve(task_count);
  for (std::size_t i = 0; i < task_count; ++i) {
    const auto word = static_cast<std::int32_t>(kHeapWord + static_cast<std::int32_t>(i * 512));
    programs.push_back(make_store_program(static_cast<std::int32_t>(i + 11), word));
    epoch.tasks[i].output_regions.clear();
    epoch.tasks[i].output_regions.push_back(OutputRegion{
        static_cast<std::uint64_t>(word), 1, true});
  }

  for (auto _ : state) {
    std::vector<DpeTaskResult> results(task_count);

    if (worker_count <= 1) {
      t81::dpe::DpeTaskRunner runner;
      for (std::size_t i = 0; i < task_count; ++i) {
        results[i] = runner.run_direct(epoch.tasks[i], programs[i]);
      }
    } else {
      t81::dpe::DpeThreadPool pool(worker_count);
      t81::dpe::DpeTaskRunner runner;
      for (std::size_t i = 0; i < task_count; ++i) {
        const std::size_t idx = i;
        bool submitted = pool.submit([&runner, &epoch, &programs, &results, idx]() {
          results[idx] = runner.run_direct(epoch.tasks[idx], programs[idx]);
        });
        benchmark::DoNotOptimize(static_cast<int>(submitted));
      }
      pool.wait_idle();
    }

    std::vector<TaskDeltaSet> delta_sets;
    delta_sets.reserve(task_count);
    for (std::size_t i = 0; i < task_count; ++i) {
      TaskDeltaSet ds;
      ds.id = t81::dpe::compute_task_id(epoch.tasks[i]);
      ds.faulted = !results[i].halted;
      ds.records = results[i].delta_records;
      delta_sets.push_back(std::move(ds));
    }

    auto commit = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(commit);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["workers"] = static_cast<double>(worker_count);
}
BENCHMARK(BM_DPE_EpochHash_SequentialVsPooled)->Args({4, 1})->Args({4, 2})->Args({16, 1})->Args({16, 4});

static void BM_DPE_SequentialEpoch_Independent(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const auto epoch = make_epoch_for_store_programs(task_count, kHeapWord);
  const auto programs = make_store_programs(task_count, kHeapWord, 100);

  for (auto _ : state) {
    auto delta_sets = run_epoch_sequential(epoch, programs);
    auto commit = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(commit);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["workers"] = 1.0;
  state.SetLabel("independent epoch sequential");
}
BENCHMARK(BM_DPE_SequentialEpoch_Independent)->Arg(4)->Arg(16)->Arg(64);

static void BM_DPE_PooledEpoch_Independent(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const std::size_t worker_count = static_cast<std::size_t>(state.range(1));
  const auto epoch = make_epoch_for_store_programs(task_count, kHeapWord);
  const auto programs = make_store_programs(task_count, kHeapWord, 100);

  for (auto _ : state) {
    auto delta_sets = run_epoch_pooled(epoch, programs, worker_count);
    auto commit = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(commit);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["workers"] = static_cast<double>(worker_count);
  state.SetLabel("independent epoch pooled");
}
BENCHMARK(BM_DPE_PooledEpoch_Independent)->Args({4, 2})->Args({16, 4})->Args({64, 4});

static void BM_DPE_SequentialEpoch_Chain(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const auto epoch = make_chain_epoch_for_programs(task_count, kHeapWord);
  const auto programs = make_chain_programs(task_count, kHeapWord, 100);

  for (auto _ : state) {
    auto delta_sets = run_epoch_by_levels(epoch, programs, 1);
    auto commit = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(commit);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["workers"] = 1.0;
  state.SetLabel("chain epoch sequential");
}
BENCHMARK(BM_DPE_SequentialEpoch_Chain)->Arg(4)->Arg(16)->Arg(64);

static void BM_DPE_PooledEpoch_Chain(benchmark::State& state) {
  const std::size_t task_count = static_cast<std::size_t>(state.range(0));
  const std::size_t worker_count = static_cast<std::size_t>(state.range(1));
  const auto epoch = make_chain_epoch_for_programs(task_count, kHeapWord);
  const auto programs = make_chain_programs(task_count, kHeapWord, 100);

  for (auto _ : state) {
    auto delta_sets = run_epoch_by_levels(epoch, programs, worker_count);
    auto commit = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(commit);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(task_count));
  state.counters["tasks"] = static_cast<double>(task_count);
  state.counters["workers"] = static_cast<double>(worker_count);
  state.SetLabel("chain epoch pooled");
}
BENCHMARK(BM_DPE_PooledEpoch_Chain)->Args({4, 2})->Args({16, 4})->Args({64, 4});

static void BM_DPE_SequentialEpoch_Fanout(benchmark::State& state) {
  const std::size_t width = static_cast<std::size_t>(state.range(0));
  const auto epoch = make_fanout_epoch_for_programs(width, kHeapWord);
  const auto programs = make_fanout_programs(width, kHeapWord, 100);

  for (auto _ : state) {
    auto delta_sets = run_epoch_by_levels(epoch, programs, 1);
    auto commit = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(commit);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(epoch.tasks.size()));
  state.counters["tasks"] = static_cast<double>(epoch.tasks.size());
  state.counters["workers"] = 1.0;
  state.SetLabel("fanout epoch sequential");
}
BENCHMARK(BM_DPE_SequentialEpoch_Fanout)->Arg(4)->Arg(16)->Arg(64);

static void BM_DPE_PooledEpoch_Fanout(benchmark::State& state) {
  const std::size_t width = static_cast<std::size_t>(state.range(0));
  const std::size_t worker_count = static_cast<std::size_t>(state.range(1));
  const auto epoch = make_fanout_epoch_for_programs(width, kHeapWord);
  const auto programs = make_fanout_programs(width, kHeapWord, 100);

  for (auto _ : state) {
    auto delta_sets = run_epoch_by_levels(epoch, programs, worker_count);
    auto commit = t81::dpe::commit_epoch(epoch, delta_sets);
    benchmark::DoNotOptimize(commit);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(epoch.tasks.size()));
  state.counters["tasks"] = static_cast<double>(epoch.tasks.size());
  state.counters["workers"] = static_cast<double>(worker_count);
  state.SetLabel("fanout epoch pooled");
}
BENCHMARK(BM_DPE_PooledEpoch_Fanout)->Args({4, 2})->Args({16, 4})->Args({64, 4});

}  // namespace
