#include <benchmark/benchmark.h>

static void bench_1(benchmark::State& state) {
    for (auto _ : state) {
    }

    // ASK
    // for (auto _ : state) {
    //     bool val = h.contains(p2);
    //     benchmark::DoNotOptimize(val);
    // }
}

BENCHMARK(bench_1);

static void bench_2(benchmark::State& state) {
    for (auto _ : state) {
    }
}

BENCHMARK(bench_2);

static void bench_3(benchmark::State& state) {
    for (auto _ : state) {
    }
}

BENCHMARK(bench_3);