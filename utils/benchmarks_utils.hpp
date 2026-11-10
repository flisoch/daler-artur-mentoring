#pragma once

#include <ctime>
#include <sched.h> 
#include <stdexcept>

void clean_up_cpu_cache()
{
    // my L3 cache is 12 MiB, so I allocate 36 MiB of data and iterate through it
    // int64_t = 8 bytes, 25 MiB = 36 * 1024 * 1024 bytes = ~ 4.5 million elements + 1 (LLM said to add +1)

    std::vector<int64_t> garbage(36 * 1024 * 128 + 1, 0xDEADBEEF);

    // L3 critical stride is 768 KB. If I hit only one cache set, It gives pollution to only 1 cache set. So I add +64 bytes to my Critical Stride to hit next cache set on every iteration
    // (768 * 1024 + 64) / 8 = 98312
    volatile int64_t sum = 0;
    size_t current_index = 0;

    for (size_t total_reads = 0; total_reads < garbage.size(); ++total_reads)
    {
        sum += garbage[current_index];
        current_index = (current_index + 98312) % garbage.size();
    }

    asm volatile("": "+r" (sum));

    /*
    $ perf stat -e LLC-loads,LLC-load-misses,page-faults ./a.out

    Performance counter stats for './a.out':

         4,402,288      LLC-loads                                                             
         3,771,147      LLC-load-misses        // ~85%                                    
             9,355      page-faults            // 9216 pages need to allocate 36 MiB vector if page=4kb
    */
}

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

template <typename T>
Percentiles get_percentiles(std::vector<T>& latencies)
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

class Timer
{
public:
    void start()
    {
        clock_gettime(CLOCK_MONOTONIC, &start_time_);
    }

    void end()
    {
        clock_gettime(CLOCK_MONOTONIC, &end_time_);
    }

    // in nanoseconds
    size_t elapsed_time()
    {
        return (end_time_.tv_sec - start_time_.tv_sec) * 1'000'000'000 
            + (end_time_.tv_nsec - start_time_.tv_nsec);
    }

private:
    timespec start_time_;
    timespec end_time_;
};