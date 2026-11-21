/*
1. Дан вектор строк, в каждой строке ид товара, количество и цена. Вот так: 123:100:200. 
Некоторые строки сломаны (нет ид, количества, цены, нет разделителей, пустая строка, ну в общем что-то с ними не так).
Задача - для всех строк, в которых больше 3 символов, сделать следующее:
* распарсить ид, количество и цену в структурку
* применить ко всем структуркам которые получились скидку 50%
* для всех кого не распарсили вернуть nullopt
* всех вывести на экран или n/a если на каком-то шаге получилась ошибка
Это задание на знание C++23, нужно широко пользоваться вьюхами, ренджами возможно, и std::optional:: and_then, or_else, transform, value_or
*/

#include <algorithm>
#include <cstdint>
#include <charconv>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <optional>
#include <iostream>

struct MarketData
{
    int64_t id{0};
    int64_t quantity{0};
    int64_t price{0};

    friend std::ostream& operator<<(std::ostream& os, const MarketData& md)
    {
        os 
            << "id: " << md.id << ", "
            << "quantity: " << md.quantity << ", "
            << "price: " << md.price;
        return os;
    }
};

template<typename Subrange>
std::optional<Subrange> read_next_val(Subrange subrange, int64_t& field)
{
    if (subrange.empty())
    {
        return std::nullopt;
    }

    std::string_view str{subrange.front()};
    auto last = str.data() + str.size();
    auto [ptr, ec] = std::from_chars(str.data(), last, field);

    return (ec == std::errc{} && ptr == last) 
        ? std::ranges::subrange(std::next(subrange.begin()), subrange.end()) 
        : std::optional<Subrange>{std::nullopt};
}

std::optional<MarketData> parse(std::string& str)
{
    if (str.size() <= 3)
    {
        return std::nullopt;
    }

    auto split = str | std::views::split(':');
    auto range = std::ranges::subrange{split.begin(), split.end()};

    MarketData md;
    
    return std::optional{range}
        .and_then([&](auto subrange){ return read_next_val(subrange, md.id);})
        .and_then([&](auto subrange){ return read_next_val(subrange, md.quantity);})
        .and_then([&](auto subrange){ return read_next_val(subrange, md.price);})
        .and_then([&](auto subrange){ return subrange.empty() ? std::optional{md} : std::nullopt;})
        .or_else([]() -> std::optional<MarketData> {return std::nullopt;});
}

std::optional<MarketData> apply_discount(std::optional<MarketData>&& market_data)
{
    if (market_data)
    {
        market_data->price *= 0.5;
    }
    return market_data;
}

int main()
{
    std::vector<std::string> market_data {
        "1:1000:200",
        "2:200:100",
        "3:400:200",
        ":4:1",
        "100200:2",
        " ",
        "",
        "123:3:400:200:1344",
        "3:400asd:200",
    };

    auto transformed = market_data
        | std::views::transform(parse)
        | std::views::transform(apply_discount);
    
    for(const auto& m: transformed)
    {
        if (m)
        {
            std::cout << *m << "\n";
        }
        else
        {
            std::cout << "n/a" << "\n";
        }
    }

    return 0;
}