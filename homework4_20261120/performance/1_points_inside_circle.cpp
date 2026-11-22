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
*/


#include <algorithm>
#include <iterator>
#include <ranges>
#include <cstddef>
#include <cstdint>
#include <random>
#include <ostream>
#include <iostream>
#include <vector>

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
    std::ranges::transform(points, std::back_inserter(soa.y), &Point::x);

    return soa;
}

void bench_aos(std::vector<Point>& points, const size_t radius)
{
    Timer timer;
    int64_t count = 0;
    timer.start();

    for (const auto& p: points)
    {
        if (radius * radius > p.x * p.x + p.y * p.y)
        {
            count++;
        };
    }
    
    timer.end();

    std::cout << count << "\n";
    std::cout << "Elapsed time: " << timer.elapsed_time() << " ns\n";
}

/*

*/
void bench_soa(Points_SoA points, const size_t radius)
{
    int64_t count = 0;

    for (size_t i = 0; i < points.x.size(); i += 4)
    {
        __m256i x = __mm256_loadu_si256(points.x[i]);

        __m256i xx = __mm256_

    }

    std::cout << count;
}


int main()
{   
    constexpr size_t R = 123;

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

    // std::ranges::for_each(points, [](const auto& p){std::cout << p << "\n";});


    return 0;
}