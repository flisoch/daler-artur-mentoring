/*
4. https://www.youtube.com/watch?v=Lrjjqc1bRHo

Определить экспериментально critical stride своей машины.
*/

// L1d = 48 KiB, 64 cache sets, 12-way, cache line = 64 bytes
// Critical stride = 64 * 64 = 4096 bytes - экспериментально это проверим

// clang++ 3_critical_stride_detection.cpp -std=c++23 -O3

/* Results:
Worst strides:
Mean latency: 2.94704 , Stride: 10240 bytes        <------- тут неожиданно затесался не кратный 4KiB
Mean latency: 2.87998 , Stride: 12288 bytes
Mean latency: 2.62591 , Stride: 4096 bytes.        <------- меньше 4KiB нет, значит это минимальный critical stride
Mean latency: 2.21295 , Stride: 8192 bytes
Mean latency: 1.31163 , Stride: 64 bytes
Best strides:
Mean latency: 1.14005 , Stride: 1920 bytes
Mean latency: 1.14006 , Stride: 1984 bytes
Mean latency: 1.14006 , Stride: 3584 bytes
Mean latency: 1.14006 , Stride: 10368 bytes
Mean latency: 1.14006 , Stride: 1792 bytes
*/

#include <cstddef>
#include <cmath>
#include <iostream>
#include <map>
#include <ranges>
#include <stdexcept>
#include <vector>
#include "../../utils/benchmarks_utils.hpp"

std::vector<size_t> make_pointer_chasing(
    size_t stride_in_bytes,
    size_t lines_per_cache_set = 12)
{
    constexpr size_t ELEMENT_SIZE = sizeof(size_t);

    const size_t stride_step = stride_in_bytes / ELEMENT_SIZE;
    const size_t indexes_count = lines_per_cache_set + 4;

    std::vector<size_t> indexes(indexes_count * stride_step + 1);

    for (size_t i = 0; i < indexes_count - 1; ++i)
    {
        indexes[i * stride_step] = (i + 1) * stride_step;
    }

    // последний на первый смотрит
    indexes[(indexes_count - 1) * stride_step] = 0;

    return indexes;
}

void test(size_t stride_in_bytes, std::multimap<double, int64_t>& latencies_by_stride)
{
    constexpr size_t ITERATIONS = 10'000'000;

    std::vector<size_t> numbers = make_pointer_chasing(stride_in_bytes);

    Timer timer;
    size_t index = 0;

    // Прогрев
    for (size_t i = 0; i < 100'000; ++i)
    {
        index = numbers[index];
    }

    timer.start();

    for (size_t i = 0; i < ITERATIONS; ++i)
    {
        index = numbers[index];
    }

    timer.end();
    asm volatile("" : : "r"(index) : "memory");
    latencies_by_stride.emplace(static_cast<double>(timer.elapsed_time()) / ITERATIONS, stride_in_bytes);
}

void print_worst_strides(std::multimap<double, int64_t>& latencies_by_stride)
{
    std::cout << "Worst strides:\n";
    for (const auto& [mean_latency, stride_in_bytes]: latencies_by_stride | std::views::reverse | std::views::take(5))
    {
        std::cout << "Mean latency: " << mean_latency << " , Stride: " << stride_in_bytes << " bytes\n";
    }
}

void print_best_strides(std::multimap<double, int64_t>& latencies_by_stride)
{
    std::cout << "Best strides:\n";
    for (const auto& [mean_latency, stride_in_bytes]: latencies_by_stride | std::views::take(5))
    {
        std::cout << "Mean latency: " << mean_latency << " , Stride: " << stride_in_bytes << " bytes\n";
    }
}

int main()
{
    std::multimap<double, int64_t> latencies_by_stride;

    for (int i = 64; i <= 12 * 1024; i += 64)
    {
        test(i, latencies_by_stride);
    }

    print_worst_strides(latencies_by_stride);
    print_best_strides(latencies_by_stride);

    return 0;
}