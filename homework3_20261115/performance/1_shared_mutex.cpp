
/*
1. Дописать shared_mutex. 
Требования: райтер ждет незавершенных ридеров, 
но пока райтер даже пытается взять лок, новые ридеры ждут снаружи; ридеры как можно меньше тормозят райтеров
*/

// clang++ 1_shared_mutex.cpp -std=c++23 -O3 -fexperimental-library -g -fsanitize=thread

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <ranges>

class shared_mutex
{
public:
    void shared_lock()
    {
        while (true)
        {
            while (is_writer_locked.load(std::memory_order_relaxed))
            {
            }

            readers_count.fetch_add(1);

            if (!is_writer_locked.load())
            {
                return;
            }
            
            readers_count.fetch_sub(1);

        }
    }

    void unique_lock()
    {
        is_writer_locked.store(true);

        while(readers_count.load() != 0)
        {}
    }

    void shared_unlock()
    {
        readers_count.fetch_sub(1);
    }

    void unique_unlock()
    {
        is_writer_locked.store(false);
    }

private:
    std::atomic<bool> is_writer_locked{false};
    std::atomic<int> readers_count{0}; 
};


void reader(std::unordered_map<std::string, int>& map, shared_mutex& mutex)
{
    int64_t local_sum = 0;
    
    for (size_t i = 0; i < 100000; ++i)
    {
        mutex.shared_lock();
        for (const auto& key : map | std::views::keys)
        {
            local_sum += map[key];
        }
        mutex.shared_unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    std::cout << local_sum << std::endl;
}

void writer(std::unordered_map<std::string, int>& map, shared_mutex& mutex)
{
    std::mt19937_64 mt(1234);

    for (size_t i = 0; i < 1000; ++i)
    {
        mutex.unique_lock();
        map.emplace(std::to_string(mt()), 1);
        mutex.unique_unlock();
    }
}


int main()
{
    shared_mutex mutex;
    std::unordered_map<std::string, int> map;
    std::jthread t1(reader, std::ref(map), std::ref(mutex));
    std::jthread t2(reader, std::ref(map), std::ref(mutex));
    std::jthread t3(reader, std::ref(map), std::ref(mutex));
    std::jthread t4(reader, std::ref(map), std::ref(mutex));
    std::jthread t_writer(writer, std::ref(map), std::ref(mutex));
}