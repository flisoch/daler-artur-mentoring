/*
Скопировать нечетные элементы из вектора в котором изначально 20 элементов в std::map, 
удалить все после 15 элемента по величине, оставшиеся вставить в новый вектор, прибавив случайное число, 
новый вектор отсортировать по убыванию, вывести на экран флаг, есть ли в нем хоть один элемент меньше 100, 
число 100 должно быть не захардкожено а передаваться функцией снаружи. 
Этот новый вектор вывести частично отсортированным (только первые сколько-то элементов),
потом из этого нового вектора поместить в unordered_map и удалить все элементы, меньшие (сумма элементов / 4).
Для каждого из этих этапов есть стандартное средство, пиши, внимательно листая cppreference (на C++20 и удели особое внимание <algorithm> и  <ranges>),
если на каком-то этапе пишешь for, значит что-то делаешь не так. Везде использовать C++-ные средства.
*/

#include <algorithm>
#include <iostream>
#include <random>
#include <ranges>
#include <vector>
#include <unordered_map>
#include <map>

template<typename Map>
void print_map(const Map& map)
{
    std::cout << "map {\n";

    std::ranges::for_each(map, [](const auto& pair)
    {
        std::cout << pair.first << ": " << pair.second << ", \n";
    });
    std::cout << "}\n";
}

template<typename T>
void print_vector(const std::vector<T>& vector)
{
    std::cout << "vector { ";
    std::ranges::for_each(vector, [](const auto& elem)
    {
        std::cout << elem << ", ";
    });

    std::cout << "}\n";
}

int64_t get_some_number()
{
    return -87;
}

int main()
{
    std::random_device rd;
    auto seed = rd();
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution dist(-100, 100);

    std::vector<int64_t> v1(20);

    std::ranges::generate(v1.begin(), v1.end(), [&dist, &gen](){ return dist(gen);});
    
    // Скопировать нечетные элементы из вектора в котором изначально 20 элементов в std::map, 
    auto evens_view = v1 
        | std::views::filter([](auto num){return num % 2 != 0;})
        | std::views::transform([](auto num){return std::pair{num, num};});

    std::map<int64_t, int64_t> m1(std::from_range, evens_view);

    print_map(m1);

    // удалить все после 15 элемента по величине, оставшиеся вставить в новый вектор, прибавив случайное число, 
    auto fifteenth_it = v1.begin() + 15;
    std::ranges::nth_element(v1, fifteenth_it);
    v1.erase(fifteenth_it, v1.end());

    std::vector<int64_t> v2;
    v2.reserve(15);
    std::ranges::transform(v1, std::back_inserter(v2), [&gen, &dist](auto num){ return num + dist(gen);});

    print_vector(v1);
    print_vector(v2);

    // новый вектор отсортировать по убыванию, вывести на экран флаг, есть ли в нем хоть один элемент меньше 100, 
    // число 100 должно быть не захардкожено а передаваться функцией снаружи. 
    std::ranges::sort(v2, std::ranges::greater());
    print_vector(v2);
    std::cout << "Is there any element less than " << get_some_number() << ": " 
        << std::ranges::any_of(v2, [](auto elem){return elem < get_some_number();})
        << "\n";
    
    // Этот новый вектор вывести частично отсортированным (только первые сколько-то элементов),
    std::ranges::partial_sort(v2, v2.begin() + 3);
    print_vector(v2);

    // потом из этого нового вектора поместить в unordered_map и удалить все элементы, меньшие (сумма элементов / 4).
    auto threshold = std::reduce(v2.begin(), v2.end(), int64_t{0}) / 4;
    std::cout << "threshold: " << threshold << "\n";

    auto to_remove = std::ranges::partition(v2, [=](auto elem){ return elem >= threshold;});
    print_vector(v2);

    auto pairs_view = to_remove | std::views::transform([](auto elem){return std::pair{elem, elem};});

    std::unordered_map<int64_t, int64_t> m2(std::from_range, pairs_view);
    v2.erase(to_remove.begin(), to_remove.end());
    
    print_vector(v2);
    print_map(m2);

    return 0;
}