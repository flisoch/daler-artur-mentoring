/*
3. Скачать реализацию SPSC array-based queue (любую). 
Запинить два потока на ядра, чтобы один писал а другой читал. 
Померять латенси доезжания значения из одного потока в другой (p50, p90, p95, p99, p99.9, p99.99, max).
Подумать как это сделать. 
Поиграть с ядрами и узнать, какие ядра на твоей машине имеют наименьшую латенси между друг другом.
*/

// #define MOODYCAMEL_CACHE_LINE_SIZE 128
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <readerwriterqueue/readerwriterqueue.h>
#include <set>
#include <string>
#include <thread>
#include <time.h>
#include <unordered_set>
#include <vector>

struct Percentiles
{
    int64_t p50;
    int64_t p90;
    int64_t p95;
    int64_t p99;
    int64_t p99_9;
    int64_t p99_99;
    int64_t max;

    friend std::ostream& operator<<(std::ostream& os, const Percentiles& p)
    {
        os << "Percentiles: {\n";
        os << "    p50: " << p.p50 << ",\n";
        os << "    p90: " << p.p90 << ",\n";
        os << "    p95: " << p.p95 << ",\n";
        os << "    p99: " << p.p99 << ",\n";
        os << "    p99.9: " << p.p99_9 << ",\n";
        os << "    p99.99: " << p.p99_99 << ",\n";
        os << "    max: " << p.max << ",\n";
        os << "}\n";

        return os;
    }
};

Percentiles getPercentiles(std::vector<int64_t>& latencies)
{
    auto percentile = [&](double p) -> int64_t 
    {
        size_t index = std::ceil(p / 100.0 * latencies.size()) - 1;
        return latencies[index];
    };

    return 
    {
        .p50 = percentile(50),
        .p90 = percentile(90),
        .p95 = percentile(95),
        .p99 = percentile(99),
        .p99_9 = percentile(99.9),
        .p99_99 = percentile(99.99),
        .max = latencies.back()
    };
}

using TValue = timespec;
using TQueue = moodycamel::ReaderWriterQueue<TValue>;

void producer(TQueue& q)
{
    timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    while (!q.try_enqueue(start_time)){};
}

void consumer(TQueue& q, std::vector<int64_t>& latencies)
{
    timespec start_time, end_time;
    while (!q.try_dequeue(start_time)){};
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int64_t latency = (end_time.tv_sec - start_time.tv_sec) * 1'000'000'000 
        + (end_time.tv_nsec - start_time.tv_nsec);

    latencies.push_back(latency);
}

int main()
{
    constexpr size_t measurementsCount = 100'000;
    std::string description = "Measuring 1 push and 1 pop, two threads, no cpu pinning, measurements count: " + std::to_string(measurementsCount);

    TQueue q(1);
    std::vector<int64_t> latencies;
    latencies.reserve(measurementsCount);

    for (size_t i = 0; i < measurementsCount; ++i)
    {
        std::thread t1(producer,  std::ref(q));
        std::thread t2(consumer, std::ref(q), std::ref(latencies));

        t1.join();
        t2.join();
    }

    std::sort(latencies.begin(), latencies.end());

    Percentiles statistics = getPercentiles(latencies);

    auto now = std::chrono::system_clock::now();

    auto timeStr = std::format("{:%F_%H-%M-%S}", now);
    std::ofstream ofs("3_statistics_" + timeStr + ".txt");
    ofs << description << std::endl;
    ofs << statistics << std::endl;
    ofs.close();

    timespec resolution;
    clock_getres(CLOCK_MONOTONIC, &resolution);

    std::cout << "Resolution: "
          << resolution.tv_sec << " s, "
          << resolution.tv_nsec << " ns\n";
}