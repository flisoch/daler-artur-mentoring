/*
4. struct Update { double volume, price }; vector<Update> updates = {...}; 
хотим сделать total = volume * price и суммировать тотал для всех апдейтов. 
Как сделать быстрее? 
А если можно менять лейаут, структуры, контейнер и т.д.?
Покопать в сторону интринсиков https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html, 
особенно fused multiply-add операций, например _mm512_fmadd_pd
*/

/*
$ clang++ -std=c++23 -O3 -march=native -ffast-math 4_soa_intrinsics.cpp
Array of structures elapsed time: 753717 ns, total = 5.49686e+11
Array of structures manual loop elapsed time: 750004 ns, total = 5.49686e+11
Structure of Arrays manual loop elapsed time: 600323 ns, total = 5.49686e+11
Structure of Arrays intrinsics elapsed time: 557474 ns, total = 5.49686e+11
*/

/*
без -ffast-math:
$ clang++ -std=c++23 -O3 -march=native 4_soa_intrinsics.cpp
Array of structures elapsed time: 810121 ns, total = 5.49686e+11
Array of structures manual loop elapsed time: 1010399 ns, total = 5.49686e+11
Structure of Arrays manual loop elapsed time: 1009062 ns, total = 5.49686e+11
Structure of Arrays intrinsics elapsed time: 527841 ns, total = 5.49686e+11
*/

#include <algorithm>
#include <immintrin.h>
#include <iostream>
#include <vector>
#include <random>
#include <ranges>
#include "../../utils/benchmarks_utils.hpp"

struct Update
{
    double volume{0};
    double price{0};
};

struct Updates
{
    std::vector<double> volumes;
    std::vector<double> prices;
};

Updates make_soa(std::vector<Update>& updates)
{
    Updates updates_soa;
    updates_soa.volumes.reserve(updates.size());
    updates_soa.prices.reserve(updates.size());

    std::ranges::transform(
        updates,
        std::back_inserter(updates_soa.volumes),
        &Update::volume
    );

    std::ranges::transform(
        updates,
        std::back_inserter(updates_soa.prices),
        &Update::price
    );

    return updates_soa;
}

void test_aos_reduce(std::vector<Update>& updates)
{
    Timer timer;
    timer.start();
    double total = std::transform_reduce(
        updates.begin(),
        updates.end(),
        0.0,
        std::plus<>{},
        [](const Update& update){ return update.volume * update.price;});
    timer.end();
    std::cout << "Array of structures elapsed time: " << timer.elapsed_time() << " ns, total = " << total << "\n";
}

void test_aos_manual_loop(std::vector<Update>& updates)
{
    /*
    с флагом -ffast-math векторизует цикл

    clang++ -std=c++23 -O3 -march=native -ffast-math \
    -Rpass=loop-vectorize \
    -Rpass-missed=loop-vectorize \
    -Rpass-analysis=loop-vectorize \
    4_soa_intrinsics.cpp

    4_soa_intrinsics.cpp:71:5: remark: vectorized loop (vectorization width: 4, interleaved count: 4)
      [-Rpass=loop-vectorize]
    71 |     for (const Update& update: updates)
             ^
    */

    Timer timer;
    timer.start();

    double total = 0.0;

    for (const Update& update: updates)
    {
        total += update.volume * update.price;
    }

    timer.end();
    std::cout << "Array of structures manual loop elapsed time: " << timer.elapsed_time() << " ns, total = " << total << "\n";
}

void test_soa_manual_loop(Updates& updates)
{
    /* 
    тоже векторизуется с -ffast-math
    */
    Timer timer;
    timer.start();

    double total = 0.0;

    for (size_t i = 0; i < updates.volumes.size(); ++i)
    {
        total += updates.volumes[i] * updates.prices[i];
    }

    timer.end();
    std::cout << "Structure of Arrays manual loop elapsed time: " << timer.elapsed_time() << " ns, total = " << total << "\n";
}

void test_soa_manual_loop_intrinsics(Updates& updates)
{
    Timer timer;
    timer.start();
   
    double total = 0.0;

    __m256d sums = _mm256_setzero_pd();

    size_t indexes = updates.volumes.size() / 4 * 4;

    for (size_t i = 0; i < indexes; i += 4)
    {
        __m256d volumes = _mm256_loadu_pd(&updates.volumes[i]);
        __m256d prices = _mm256_loadu_pd(&updates.prices[i]);

        sums = _mm256_fmadd_pd(volumes, prices, sums);
    }

    double* subtotals = new double[4];
    _mm256_storeu_pd(subtotals, sums);

    total += subtotals[0] + subtotals[1] + subtotals[2] + subtotals[3];

    for (size_t i = indexes; i < updates.volumes.size(); ++i)
    {
        total += updates.volumes[i] * updates.prices[i];
    }

    timer.end();
    std::cout << "Structure of Arrays intrinsics elapsed time: " << timer.elapsed_time() << " ns, total = " << total << "\n";
    delete[] subtotals;
}

int main()
{
    std::mt19937_64 mt(1234);
    std::uniform_real_distribution vol_dist(1.0, 1'000'000.0);
    std::uniform_real_distribution price_dist(1.0, 1.2);
    constexpr size_t N = 1'000'000;
    std::vector<Update> updates(N);

    std::ranges::generate(updates, [&vol_dist, &price_dist, &mt]()
    {
        return Update{vol_dist(mt), price_dist(mt)};
    });

    Updates updates_soa = make_soa(updates);

    test_aos_reduce(updates);
    test_aos_manual_loop(updates);

    test_soa_manual_loop(updates_soa);
    test_soa_manual_loop_intrinsics(updates_soa);
    
    return 0;
}