/*
3. Скачать реализацию SPSC array-based queue (любую). 
Запинить два потока на ядра, чтобы один писал а другой читал. 
Померять латенси доезжания значения из одного потока в другой (p50, p90, p95, p99, p99.9, p99.99, max).
Подумать как это сделать. 
Поиграть с ядрами и узнать, какие ядра на твоей машине имеют наименьшую латенси между друг другом.
*/

/*
    clang++ 3_spsc.cpp -std=c++20 -O3
    ./a.out 2 3
*/

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <latch>
#include <ostream>
#include <readerwriterqueue/readerwriterqueue.h>
#include <sched.h>
#include <set>
#include <stdexcept>
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

constexpr size_t kMeasurementsCount = 100'000;

void pin_this_thread(int cpu)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) != 0)
    {
        throw std::runtime_error("sched_setaffinity failed");
    }
}

void producer(TQueue& q, TQueue& ack_q, std::latch& both_pinned, int cpu)
{
    pin_this_thread(cpu);

    both_pinned.count_down();
    both_pinned.wait();

    timespec start_time;

    for (size_t i = 0; i < kMeasurementsCount; ++i)
    {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        // ping
        while (!q.try_enqueue(start_time)){};

        TValue ack;
        // pong
        while (!ack_q.try_dequeue(ack)){}
    }

}

void consumer(TQueue& q, TQueue& ack_q, std::vector<int64_t>& latencies, std::latch& both_pinned, int cpu)
{
    pin_this_thread(cpu);

    both_pinned.count_down();
    both_pinned.wait();

    timespec start_time, end_time;
    
    for (size_t i = 0; i < kMeasurementsCount; ++i)
    {
        // ping
        while (!q.try_dequeue(start_time)){};
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        int64_t latency = (end_time.tv_sec - start_time.tv_sec) * 1'000'000'000 
            + (end_time.tv_nsec - start_time.tv_nsec);

        latencies.push_back(latency);

        // pong
        while (!ack_q.try_enqueue(start_time)){};
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <producer_cpu> <consumer_cpu>\n";
        return EXIT_FAILURE;
    }

    int producer_cpu = std::stoi(argv[1]);
    int consumer_cpu = std::stoi(argv[2]);

    std::string description = "Measuring 1 push and 1 pop, two threads (ping-pong), with cpu pinning, no cpu isolation, measurements count: "
        + std::to_string(kMeasurementsCount)
        + "\nproducer_cpu: " + std::to_string(producer_cpu)
        + ", consumer_cpu: " + std::to_string(consumer_cpu);

    TQueue q(1);
    TQueue ack_q(1);

    std::vector<int64_t> latencies;
    latencies.reserve(kMeasurementsCount);

    std::latch both_pinned(2);

    std::thread t1(producer,  std::ref(q), std::ref(ack_q), std::ref(both_pinned), producer_cpu);
    std::thread t2(consumer, std::ref(q), std::ref(ack_q), std::ref(latencies), std::ref(both_pinned), consumer_cpu);

    t1.join();
    t2.join();

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