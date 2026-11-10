/* 
4. https://www.youtube.com/watch?v=Lrjjqc1bRHo - пробежать взглядом, 
нам отсюда нужны кэшсеты, N-way associativity, critical stride. 

Определить экспериментально critical stride своей машины.
*/

// как я понимаю, critical stride - это шаг, при котором очередной адрес относится к одному и тому же cache set
// получается, если у меня L1d cache = 48 KiB, 64 cache-set'ов, 12 кеш-линий по 64 байт на каждый кеш-сет, и каждая следующая кеш-линия относится к следующему cache-set'у,
// то чтобы попасть тот же самый cache-set, нужно 64*64 байт = 4096 bytes = 4KiB

// как я понимаю, нужно экспериментально для разных stride проверить, при каком stride средняя задержка увеличивается

#include <algorithm>
#include <iostream>
#include <random>
#include <ranges>
#include <vector>
#include "../../utils/benchmarks_utils.hpp"

template<std::integral T>
void test(size_t stride_in_bytes, std::vector<T>& numbers)
{
    // clean_up_cpu_cache();

    std::vector<int64_t> latencies;
    latencies.reserve(numbers.size());
    Timer timer;

    int64_t sum = 0;
    const size_t stride_step = stride_in_bytes / sizeof(T);
    size_t index = 0;
    for (size_t i = 0; i < numbers.size(); ++i)
    {
        timer.start();
        for (size_t j = 0; j < 16; ++j)
        {
            sum += numbers[j * stride_step];
        }
        timer.end();

        latencies.push_back(timer.elapsed_time());
    }
    std::cout << sum << "\n";

    std::sort(latencies.begin(), latencies.end());
    Percentiles p = get_percentiles(latencies);
    std::cout << "Stride " << stride_in_bytes << " bytes: \n" << p << "\n";
}


int main()
{
    size_t L1d_CACHE_SIZE = 48 * 1024; // 48 KiB
    size_t L1d_CACHE_SETS = 64;
    std::vector<int64_t> numbers(L1d_CACHE_SIZE / sizeof(int64_t) * L1d_CACHE_SETS);
    std::mt19937_64 mt(12345);

    std::ranges::generate(numbers.begin(), numbers.end(), mt);
    test(64, numbers);
    test(128, numbers);
    test(2048, numbers);

    test(4096, numbers);
    test(4097, numbers);
    
    test(8192, numbers);
    test(9999, numbers);

    return 0;
}