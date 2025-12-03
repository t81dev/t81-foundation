#include <benchmark/benchmark.h>
#include <t81/frontend/ir_generator.hpp>
#include <t81/frontend/lexer.hpp>
#include <t81/frontend/parser.hpp>
#include <t81/frontend/semantic_analyzer.hpp>
#include <t81/tisc/binary_emitter.hpp>

#include <string_view>

namespace {
constexpr std::string_view kBenchmarkSource = R"(
fn fib(n: T81Int) -> T81Int {
  var prev: T81Int = 0;
  var curr: T81Int = 1;
  var iter: T81Int = 1;
  while (iter <= n) {
    let next: T81Int = prev + curr;
    prev = curr;
    curr = next;
    iter = iter + 1;
  }
  return prev;
}

fn main() -> T81Int {
  let n: T81Int = 15;
  if (n > 10) {
    return fib(n);
  }
  return 0;
}
)";
}

static void BM_T81LangCompile_T81(benchmark::State& state) {
  for (auto _ : state) {
    t81::frontend::Lexer lexer(kBenchmarkSource);
    t81::frontend::Parser parser(lexer, "t81lang_benchmark");
    auto statements = parser.parse();
    if (parser.had_error()) {
      state.SkipWithError("parser failure");
      break;
    }

    t81::frontend::SemanticAnalyzer analyzer(statements, "t81lang_benchmark");
    analyzer.analyze();
    if (analyzer.had_error()) {
      state.SkipWithError("semantic failure");
      break;
    }

    t81::frontend::IRGenerator ir_generator;
    ir_generator.attach_semantic_analyzer(&analyzer);
    auto ir = ir_generator.generate(statements);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir);
    benchmark::DoNotOptimize(program.insns.size());
  }
  state.SetLabel("T81Lang frontend compile");
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_T81LangCompile_T81);

static void BM_T81LangCompile_Binary(benchmark::State& state) {
  constexpr int kIterations = 24;
  for (auto _ : state) {
    int a = 0;
    int b = 1;
    for (int i = 0; i < kIterations; ++i) {
      int next = a + b;
      benchmark::DoNotOptimize(next);
      a = b;
      b = next;
    }
    benchmark::DoNotOptimize(b);
  }
  state.SetLabel("Binary Fibonacci loop");
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_T81LangCompile_Binary);
