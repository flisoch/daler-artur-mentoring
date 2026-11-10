/*
Дописать формулу задержек при чтении матрицы построчно или постолбцово. 
Почитать большую матрицу построчно и постолбцово, 
померять clock_gettime(CLOCK_MONOTONIC). 
Учти что построчный прогон может засорить кэши для постолбцового, и наоборот. 
Возможно понадобится явный вызов cacheflush() или между прогонами интенсивно почитать-пописать какой-нибудь мусор.
*/

// clang++ 2_matrix_traversal.cpp --std=c++20 -O3
/*
    m4 latencies:
        L1 = 0.67ns
        L2 = 1.5ns  
        L3 = 20ns
        System-Level Cache (SLC) = 30ns
        Main Memory (RAM) = 50ns
    
    Cache Line Size = 128 bytes

    total latency = count / CLS * L1 + count / CLS * RAM
*/

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <random>
#include <type_traits>
#include <vector>
#include <time.h>

template<typename T>
using TMatrix = std::vector<T>;

template<std::integral T>
TMatrix<T> getMatrix(const size_t rowsCount, const size_t columnsCount)
{
    TMatrix<T> matrix;

    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_int_distribution<T> dist(-1000, 1000);

    matrix.reserve(rowsCount * columnsCount);

    for (size_t i = 0; i < rowsCount; ++i)
    {        
        for (size_t j = 0; j < columnsCount; ++j)
        {
            matrix.push_back(dist(generator));
        }
    }

    return matrix;
}

int main()
{
    constexpr size_t N = 10'000;
    constexpr size_t M = 10'000;

    std::cout << "Generating " << N << "x" << M << " matrix..." << std::endl;
    TMatrix<int64_t> matrix = getMatrix<int64_t>(N, M);

    int64_t sum = 0;

    timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (size_t i = 0; i < N; ++i)
    {
        for (size_t j = 0; j < M; ++j)
        {
            sum += matrix[i * M + j];
        }
    }
    // for (size_t j = 0; j < M; ++j)
    // {
    //     for (size_t i = 0; i < N; ++i)
    //     {
    //         sum += matrix[i * M + j];
    //     }
    // }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    const int64_t elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1'000'000'000
        + (end_time.tv_nsec - start_time.tv_nsec);

    std::cout << "sum: " << sum << std::endl;
    std::cout << "elapsed time: " << elapsed_time << std::endl;
}