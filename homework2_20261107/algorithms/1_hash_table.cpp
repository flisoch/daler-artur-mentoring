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

#include <bitset>
#include <iostream>
#include <time.h>
#include <unordered_map>
#include <utility>
#include <vector>
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

void print_elapsed_time(timespec& start, timespec& end)
{
    const int64_t elapsed_ns = (end.tv_sec - start.tv_sec) * 1'000'000'000LL +
        (end.tv_nsec - start.tv_nsec);

    std::cout << "Elapsed time: "
        << elapsed_ns << " ns ("
        << elapsed_ns / 1'000'000.0 << " ms)\n";
}



int main()
{
    clean_up_cpu_cache();
    // std::unordered_map<int, int> umap;
    // hash_table<int, int, 1'000'003> table;

    // auto load_capacity = 0.5 * table.capacity();

    // timespec start_time, end_time;

    // std::cout << "Insert: \n";
    // clock_gettime(CLOCK_MONOTONIC, &start_time);

    // for (size_t i = 0; i < load_capacity; ++i)
    // {
    //     table.insert(i * 2, i);
    // }

    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    // print_elapsed_time(start_time, end_time);

    // clean_up_cache();
    
    // clock_gettime(CLOCK_MONOTONIC, &start_time);
    // for (size_t i = 0; i < load_capacity; ++i)
    // {
    //     umap.emplace(i * 2, i);
    // }
    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    // print_elapsed_time(start_time, end_time);

    // clean_up_cache();

    // int64_t sum = 0;

    // std::cout << "Lookup: \n";
    // clock_gettime(CLOCK_MONOTONIC, &start_time);
    // for (size_t i = 0; i < load_capacity; ++i)
    // {
    //     auto& pair = table.lookup(i * 2);

    //     sum += pair.second;
    // }
    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    // print_elapsed_time(start_time, end_time);
    
    // clean_up_cache();
    // std::cout << "Hash table sum: " << sum << '\n';
    // sum = 0;

    // clock_gettime(CLOCK_MONOTONIC, &start_time);
    // for (size_t i = 0; i < load_capacity; ++i)
    // {
    //     auto& elem = umap.at(i * 2);

    //     sum += elem;
    // }
    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    // print_elapsed_time(start_time, end_time);
    // std::cout << "Hash table sum: " << sum << '\n';

    // std::cout << "Erase: \n";
    // clock_gettime(CLOCK_MONOTONIC, &start_time);
    // for (size_t i = 0; i < load_capacity; ++i)
    // {
    //     if (!table.erase(i * 2))
    //     {
    //         std::cout << "Couldn't erase key: " << i << "\n";
    //     }
    // }
    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    // print_elapsed_time(start_time, end_time);


    // clock_gettime(CLOCK_MONOTONIC, &start_time);
    // for (size_t i = 0; i < load_capacity; ++i)
    // {
    //     if (!umap.erase(i * 2))
    //     {
    //         std::cout << "Couldn't erase key: " << i << "\n";
    //     }
    // }
    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    // print_elapsed_time(start_time, end_time);

    return 0;
}