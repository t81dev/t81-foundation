#include <benchmark/benchmark.h>
#include <cstdint>
#include <vector>

static void BM_MemoryBandwidth_ReadWrite(benchmark::State& state) {
    const std::size_t buffer_size = static_cast<std::size_t>(state.range(0));
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

    const uint64_t bytes = buffer_size * 2; // read + write
    state.SetBytesProcessed(state.iterations() * bytes);
    state.counters["bytes_per_second"] =
        benchmark::Counter(static_cast<double>(state.bytes_processed()),
                           benchmark::Counter::kIsRate);
    state.SetLabel("Streaming read/write");
}

BENCHMARK(BM_MemoryBandwidth_ReadWrite)->Arg(16 * 1024 * 1024);
