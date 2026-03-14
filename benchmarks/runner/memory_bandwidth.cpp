#include <benchmark/benchmark.h>
#include <cstdint>
#include <string>
#include <vector>

static void BM_MemoryBandwidth_ReadWrite_T81(benchmark::State& state) {
    const std::size_t buffer_size = static_cast<std::size_t>(state.range(0));
    const uint64_t bytes = buffer_size * 2; // read + write
    std::vector<uint8_t> buffer(buffer_size);
    for (std::size_t i = 0; i < buffer_size; ++i) {
        buffer[i] = static_cast<uint8_t>(i);
    }

    for (auto _ : state) {
        uint64_t acc = 0;
        for (std::size_t i = 0; i < buffer_size; ++i) {
            acc += buffer[i];
            buffer[i] = static_cast<uint8_t>(acc);
        }
        benchmark::DoNotOptimize(acc);
    }

    state.counters["work_per_iter"] = static_cast<double>(bytes);
    state.SetBytesProcessed(state.iterations() * bytes);
    state.counters["bytes_per_second"] =
        benchmark::Counter(static_cast<double>(state.bytes_processed()),
                           benchmark::Counter::kIsRate);
    state.SetLabel("comparison=apples-to-apples; work: bytes/iter=" + std::to_string(bytes));
}
BENCHMARK(BM_MemoryBandwidth_ReadWrite_T81)->Arg(16 * 1024 * 1024)->Repetitions(3);

static void BM_MemoryBandwidth_ReadWrite_Binary(benchmark::State& state) {
    const std::size_t buffer_size = static_cast<std::size_t>(state.range(0));
    const uint64_t bytes = buffer_size * 2; // read + write
    std::vector<uint8_t> buffer(buffer_size);
    for (std::size_t i = 0; i < buffer_size; ++i) {
        buffer[i] = static_cast<uint8_t>(i);
    }

    for (auto _ : state) {
        uint64_t acc = 0;
        for (std::size_t i = 0; i < buffer_size; ++i) {
            acc += buffer[i];
            buffer[i] = static_cast<uint8_t>(acc);
        }
        benchmark::DoNotOptimize(acc);
    }

    state.counters["work_per_iter"] = static_cast<double>(bytes);
    state.SetBytesProcessed(state.iterations() * bytes);
    state.counters["bytes_per_second"] =
        benchmark::Counter(static_cast<double>(state.bytes_processed()),
                           benchmark::Counter::kIsRate);
    state.SetLabel("comparison=apples-to-apples; work: bytes/iter=" + std::to_string(bytes));
}
BENCHMARK(BM_MemoryBandwidth_ReadWrite_Binary)->Arg(16 * 1024 * 1024)->Repetitions(3);
