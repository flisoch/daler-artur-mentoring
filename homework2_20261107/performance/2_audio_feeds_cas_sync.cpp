/*
Синхронизация аудиопотоков.
В двух потоках много раз в секунду вызывается функция feed(int val), она должна делать max_val = max(max_val, val),
но при этом один поток не должен случайно сломать эту логику другому.
Пример: первый поток проверил что val > max_val, пытается поменять, 
в этот момент второй отбирает управление, тоже проверяет, меняет, выходит, 
первый поток возвращает управление и заканчивает свое изменение, 
так вот в этом сценарии первый поток может перезаписать большее значение меньшим. 
надо это починить через CAS (см. cppreference - compare_exchange_strong). 
Барьеры памяти не ставить, оно само поставит пока что. 
Написать стресс-тест на корректность.
*/

// clang++ 2_audio_feeds_cas_sync.cpp -std=c++23 -O3

#include <algorithm>
#include <iostream>
#include <random>
#include <ranges>
#include <thread>
#include <vector>

void feed(std::atomic<int64_t>& max, int64_t value)
{
    int64_t expected = max.load();

    while (value > expected)
    {
        if (max.compare_exchange_strong(expected, value))
        {
            break;
        }
    }
}

void feed_all(std::vector<int64_t>& v, std::atomic<int64_t>& max)
{
    std::ranges::for_each(v, [&max](const auto& num)
    {
        feed(max, num);
    });
}

int main()
{
    constexpr size_t N = 100'000;
    std::random_device rd;
    std::mt19937_64 mt(rd());
    std::uniform_int_distribution dist(-1'000'000, 1'000'000);
    std::vector<int64_t> v1(N);
    std::vector<int64_t> v2(N);

    auto rand_int = [&mt, &dist](){return dist(mt);};
    std::ranges::generate(v1.begin(), v1.end(), rand_int);
    std::ranges::generate(v2.begin(), v2.end(), rand_int);

    std::cout << "v1 max: " << std::ranges::max(v1) << "\n";
    std::cout << "v2 max: " << std::ranges::max(v2) << "\n";

    std::atomic<int64_t> max = -1001;
    
    std::thread t1(feed_all, std::ref(v1), std::ref(max));
    std::thread t2(feed_all, std::ref(v2), std::ref(max));

    t1.join();
    t2.join();
    
    std::cout << "max: " << max.load() << "\n";
    return 0;
}