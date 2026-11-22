/*
1. Почитать про SIMD, SSE, AVX
Окинуть взглядом вот это https://www.intel.com/content/dam/develop/external/us/en/documents/18072-347603.pdf
Count Points Inside Circle - написать интринсик (SoA) и нон-интринсик (AoS) версии, побенчить.
Ускорить SoA вариант интринсиками отсюда https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#avxnewtechs=FMA
Побенчить. Заглянуть на godbolt, убедиться что в SoA варианте используются векторные инструкции
*/


/*              
                y
                ^
   *            |           *
            .---+---.
         .-'    |    '-.
        /   *   |       \
       /        |   *    \
------+---------+---------+------> x
       \    *   |        /
        \       |       /    *
         '-.    |    .-'
            '---+---'
    *           |

    r^2 = x^2 + y^2
    Is inside = r^2 > x^2 + y^2

    100k elements: 

        Total count AoS: 29632
        Elapsed time: 51680 ns
        Total count SoA: 29632
        Elapsed time: 44647 ns
        Total count SoA FMA: 29632
        Elapsed time: 46601 ns


        Total count AoS: 29632
        Elapsed time: 49969 ns
        Total count SoA: 29632
        Elapsed time: 53690 ns
        Total count SoA FMA: 29632
        Elapsed time: 45192 ns
    
    10k elements:

        Total count AoS: 3006
        Elapsed time: 6127 ns
        Total count SoA: 3006
        Elapsed time: 3656 ns
        Total count SoA FMA: 3006
        Elapsed time: 4012 ns

        Total count AoS: 3006
        Elapsed time: 4854 ns
        Total count SoA: 3006
        Elapsed time: 2834 ns
        Total count SoA FMA: 3006
        Elapsed time: 2641 ns

    clang++ 1_points_inside_circle.cpp -std=c++23 -march=native -O3 -ffast-math
*/


#include <algorithm>
#include <bit>
#include <iterator>
#include <memory>
#include <ranges>
#include <cstddef>
#include <cstdint>
#include <random>
#include <ostream>
#include <iostream>
#include <vector>
#include <immintrin.h>

#include "../../utils/benchmarks_utils.hpp"

struct Point
{
    int64_t x{0};
    int64_t y{0};

    friend std::ostream& operator<<(std::ostream& os, const Point& p)
    {
        os << "Point: { X=" << p.x << ", Y=" << p.y << " }";
        return os;
    }
};

struct Points_SoA
{
    std::vector<int64_t> x;
    std::vector<int64_t> y;
};

Points_SoA to_soa(const std::vector<Point>& points)
{
    Points_SoA soa;

    soa.x.reserve(points.size());
    soa.y.reserve(points.size());

    std::ranges::transform(points, std::back_inserter(soa.x), &Point::x);
    std::ranges::transform(points, std::back_inserter(soa.y), &Point::y);

    return soa;
}

void bench_aos(std::vector<Point>& points, const int64_t radius)
{
    int64_t count = 0;
    Timer timer;
    timer.start();

    for (const auto& p: points)
    {
        if (radius * radius > p.x * p.x + p.y * p.y)
        {
            count++;
        };
    }
    
    timer.end();

    std::cout << "Total count AoS: " << count << "\n";
    std::cout << "Elapsed time: " << timer.elapsed_time() << " ns\n";
}


void bench_soa(Points_SoA points, const int64_t radius)
{
    int64_t total_count = 0;

    Timer timer;
    timer.start();

    auto rr_scalar = radius * radius;
    __m256 rr = _mm256_set1_epi64x(rr_scalar);

    __m256 count_4 = _mm256_setzero_si256();

    size_t i = 0;
    for (; i < points.x.size(); i += 4)
    {
        __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&points.x[i]));
        const auto xx = _mm256_mul_epi32(x, x);

        __m256i y = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&points.y[i]));
        const auto yy = _mm256_mul_epi32(y, y);

        const auto sum_xxyy = _mm256_add_epi64(xx, yy);

        const auto is_inside = _mm256_cmpgt_epi64(rr, sum_xxyy);

        count_4 = _mm256_sub_epi64(count_4, is_inside); // - (-1) или - 0
    }

    int64_t counts[4];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(counts), count_4);
    total_count = counts[0] + counts[1] + counts[2] + counts[3];

    for (; i < points.x.size(); ++i)
    {
        if (rr_scalar > (points.x[i] * points.x[i] + points.y[i] * points.y[i]))
        {
            ++total_count;
        }
    }

    timer.end();
    std::cout << "Total count SoA: " << total_count << "\n";
    std::cout << "Elapsed time: " << timer.elapsed_time() << " ns\n";
}

void bench_soa_fma(Points_SoA points, const int64_t radius)
{
    const std::vector<double> xs(points.x.begin(), points.x.end());
    const std::vector<double> ys(points.y.begin(), points.y.end());

    int64_t total_count = 0;

    Timer timer;
    timer.start();

    const double r = static_cast<double>(radius);
    const double rr_scalar = r * r;
    const __m256d rr = _mm256_set1_pd(rr_scalar);

    __m256i count_4 = _mm256_setzero_si256();

    std::size_t i = 0;
    for (; i < xs.size(); i += 4)
    {
        const __m256d x = _mm256_loadu_pd(&xs[i]);
        const __m256d y = _mm256_loadu_pd(&ys[i]);

        const auto yy = _mm256_mul_pd(y, y);
        const auto sum_xxyy = _mm256_fmadd_pd(x, x, yy);
        const auto is_inside = _mm256_cmp_pd(rr, sum_xxyy, _CMP_GT_OQ);

        count_4 = _mm256_sub_epi64(count_4, _mm256_castpd_si256(is_inside));
    }

    int64_t counts[4];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(counts), count_4);
    total_count = counts[0] + counts[1] + counts[2] + counts[3];

    for (; i < xs.size(); ++i)
    {
        if (rr_scalar > xs[i] * xs[i] + ys[i] * ys[i])
        {
            ++total_count;
        }
    }

    timer.end();
    std::cout << "Total count SoA FMA: " << total_count << '\n';
    std::cout << "Elapsed time: " << timer.elapsed_time() << " ns\n";
}

int main()
{   
    constexpr int64_t R = 123;

    std::mt19937_64 mt(1234);
    std::uniform_int_distribution<int64_t> distr(-200, 200);

    std::vector<Point> points(10000);


    std::ranges::generate(points.begin(),
        points.end(), 
        [&mt, &distr]()
            {
                return Point{distr(mt), distr(mt)};
            });

    bench_aos(points, R);

    bench_soa(to_soa(points), R);
    bench_soa_fma(to_soa(points), R);

    return 0;
}