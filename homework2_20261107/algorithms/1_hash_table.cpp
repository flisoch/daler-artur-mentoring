/*
1. Почитать про хеш-таблицы с открытой и закрытой адресацией.
Написать хеш-таблицу с открытой адресацией фиксированной емкости (пусть будет 1e6 элементов),
поддерживаемые операции: lookup(K) -> pair<K, V>&, insert(K, V) -> pair<bool, pair<K, V>>&, 
erase(K) -> bool, size() -> int.
Написать для нее стресс-тест, который будет что-то интенсивно вставлять, искать и удалять.
Сравнить с std::unordered_map.
Ты уже знаешь тайминги доступа к памяти, поэтому на вопрос как правильно мерять
(где-то по 1 операции, где-то рядами потому что операция стоит дешевле измерения) у тебя во многом есть ответ.
*/

#include <algorithm>
#include <bitset>
#include <iostream>
#include <random>
#include <time.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <unordered_set>
#include <limits>

#include "../../utils/benchmarks_utils.hpp"

// хеш таблица открытой адресацией фиксированной емкости, linear probing + backward shift deletion
template<
    typename Key,
    typename Value,
    size_t Capacity = 1'000'000,
    typename Hash = std::hash<Key>,
    typename Allocator = std::allocator<std::pair<Key, Value>>
>
class hash_table
{
    using Pair = std::pair<Key, Value>;
public:
    hash_table()
    {
        slots_ = std::allocator_traits<Allocator>::allocate(allocator_, Capacity);
    }

    hash_table(const hash_table&) = delete;
    hash_table& operator=(const hash_table&) = delete;

    ~hash_table()
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            if (is_occupied(i))
            {
                std::allocator_traits<Allocator>::destroy(allocator_, slots_ + i);
                occupied_slots_.reset(i);
            }
        }

        std::allocator_traits<Allocator>::deallocate(allocator_, slots_, Capacity);
    }
    
    Pair& lookup(Key key)
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            const size_t index = (hash_(key) + i) % Capacity;

            if (!is_occupied(index))
            {
                throw std::runtime_error("Key is not present in table");
            }

            if (slots_[index].first == key)
            {
                return slots_[index];
            }
        }

        throw std::runtime_error("Key is not present in table");
    }

    std::pair<bool, Pair&> insert(Key key, Value value)
    { 
        for (size_t i = 0; i < Capacity; ++i)
        {
            const size_t index = (hash_(key) + i) % Capacity;
            if (is_occupied(index))
            {
                if (slots_[index].first == key)
                {
                    return std::pair<bool, Pair&>{false, slots_[index]};
                }

                continue;
            }

            std::allocator_traits<Allocator>::construct(allocator_, slots_ + index, key, value);
            occupied_slots_.set(index);
            size_++;
            return std::pair<bool, Pair&>{true, slots_[index]};
        }

        throw std::runtime_error("Hash table is full");
    }

    bool erase(Key key)
    {
        size_t deleted_slot = 0;
        bool is_deleted = false;

        for (size_t i = 0; i < Capacity; ++i)
        {
            const size_t index = (hash_(key) + i) % Capacity;

            if ((!is_occupied(index)))
            {
                return false;
            }

            if (slots_[index].first == key)
            {
                std::allocator_traits<Allocator>::destroy(allocator_, slots_ + index);
                size_--;
                occupied_slots_.reset(index);
                deleted_slot = index;
                is_deleted = true;
                break;
            }
        }

        if (!is_deleted)
        {
            return false;
        }

        // backward shift
        size_t current_slot = (deleted_slot + 1) % Capacity;

        for (size_t i = 0; i < Capacity - 1; ++i)
        {   
            if (!is_occupied(current_slot))
            {
                return is_deleted;
            }

            const size_t initial_slot = hash_(slots_[current_slot].first) % Capacity;
            
            const size_t probing_steps_to_deleted = (deleted_slot + Capacity - initial_slot) % Capacity;
            const size_t probing_steps_to_current = (current_slot + Capacity - initial_slot) % Capacity;

            bool should_move = probing_steps_to_deleted < probing_steps_to_current;

            if (should_move)
            {
                std::allocator_traits<Allocator>::construct(allocator_, slots_ + deleted_slot, slots_[current_slot]);
                std::allocator_traits<Allocator>::destroy(allocator_, slots_ + current_slot);
                occupied_slots_.set(deleted_slot);
                occupied_slots_.reset(current_slot);
                deleted_slot = current_slot;
            }

            current_slot = (current_slot + 1) % Capacity;
        }

        return is_deleted;
    }

    size_t size() const
    {
        return size_;
    }

    size_t capacity() const
    {
        return Capacity;
    }

private:

    bool is_occupied(const size_t index) const
    {
        return occupied_slots_.test(index);
    }

private:
    [[no_unique_address]] Allocator allocator_;
    [[no_unique_address]] Hash hash_;

    Pair* slots_ = nullptr;
    std::bitset<Capacity> occupied_slots_; // ~125 KB

    size_t size_{0};   
};

std::vector<int> generate_keys(const size_t N, std::mt19937_64& mt)
{
    std::vector<int> keys;
    keys.reserve(N);

    std::unordered_set<int> unique_keys;
    unique_keys.reserve(N);

    std::uniform_int_distribution<int> dist(
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max()
    );

    while (keys.size() < N)
    {
        int key = dist(mt);

        if (unique_keys.insert(key).second)
        {
            keys.push_back(key);
        }
    }

    return keys;
}

template<size_t LoadPercent>
void run_tests(const std::vector<int>& keys,
               const std::vector<int>& values)
{
    constexpr size_t N = 1'000'000;
    constexpr size_t Capacity = (N * 100 + LoadPercent - 1) / LoadPercent;

    std::unordered_map<int, int> umap;
    umap.max_load_factor(1.0f);
    umap.rehash(Capacity);

    hash_table<int, int, Capacity> table;

    Timer timer;

    std::cout << "\nLoad factor: "
        << static_cast<double>(N) / Capacity
        << " (capacity = " << Capacity << ")\n";

    std::cout << "Insert: \n";
    timer.start();

    for (size_t i = 0; i < N; ++i)
    {
        table.insert(keys[i], values[i]);
    }
    timer.end();

    std::cout << "hash_table mean time: "
        << timer.elapsed_time() / N << " ns ("
        << timer.elapsed_time() / N / 1'000'000.0 << " ms)\n";

    clean_up_cpu_cache();
    
    timer.start();
    for (size_t i = 0; i < N; ++i)
    {
        umap.emplace(keys[i], values[i]);
    }
    timer.end();
    std::cout << "std::unordered_map mean time: "
        << timer.elapsed_time() / N << " ns ("
        << timer.elapsed_time() / N / 1'000'000.0 << " ms)\n";

    clean_up_cpu_cache();

    int64_t sum = 0;

    std::cout << "\nLookup: \n";
    timer.start();
    for (size_t i = 0; i < N; ++i)
    {
        auto& pair = table.lookup(keys[i]);

        sum += pair.second;
    }
    timer.end();

    std::cout << "hash_table mean time: "
        << timer.elapsed_time() / N << " ns ("
        << timer.elapsed_time() / N / 1'000'000.0 << " ms)\n";

    
    clean_up_cpu_cache();
    std::cout << "sum: " << sum << '\n';
    sum = 0;

    timer.start();
    for (size_t i = 0; i < N; ++i)
    {
        auto& elem = umap.at(keys[i]);
        sum += elem;
    }
    timer.end();

    std::cout << "std::unordered_map mean time: "
        << timer.elapsed_time() / N << " ns ("
        << timer.elapsed_time() / N / 1'000'000.0 << " ms)\n";

    std::cout << "sum: " << sum << '\n';

    std::cout << "\nErase: \n";
    
    timer.start();
    for (size_t i = 0; i < N; ++i)
    {
        table.erase(keys[i]);
    }
    timer.end();

    std::cout << "hash_table mean time: "
        << timer.elapsed_time() / N << " ns ("
        << timer.elapsed_time() / N / 1'000'000.0 << " ms)\n";


    timer.start();
    for (size_t i = 0; i < N; ++i)
    {
        umap.erase(keys[i]);
    }
    timer.end();

    std::cout << "std::unordered_map mean time: "
        << timer.elapsed_time() / N << " ns ("
        << timer.elapsed_time() / N / 1'000'000.0 << " ms)\n";

}
int main()
{
    clean_up_cpu_cache();
    
    constexpr size_t N = 1'000'000;
    std::vector<int64_t> latencies;
    latencies.reserve(N);
    std::mt19937_64 mt(12345);

    Timer timer;
    std::cout << "Timer latency:\n"; // 13-14 ns, max: 30'678 ns
    for (size_t i = 0; i < N; ++i)
    {
         timer.start();
         timer.end();
         latencies.push_back(timer.elapsed_time());
    }
   
    Percentiles percentiles = get_percentiles(latencies);
    std::cout << percentiles << "\n";

    std::vector<int> keys = generate_keys(N, mt);
    std::vector<int> values(N);
    std::ranges::generate(values.begin(), values.end(), [&mt](){return mt();});

    run_tests<50>(keys, values);  // load factor
    run_tests<60>(keys, values);
    run_tests<70>(keys, values);
    run_tests<80>(keys, values);
    run_tests<85>(keys, values);
    run_tests<90>(keys, values);
    run_tests<95>(keys, values);
    run_tests<98>(keys, values);
    run_tests<100>(keys, values);
    
    return 0;
}