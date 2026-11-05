/*
1. Почитать про strong exception guarantee. 
Написать move-aware вектор. Не нужно писать прямо все функции вектора, просто по минимуму чтобы он делал push_back с ресайзом. 
Потестить что после исключения в конструкторе перемещения / копирования он возвращается в исходное состояние.
*/

#include <cstddef>
#include <exception>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <vector>

template<typename T, typename Allocator = std::allocator<T>>
class my_vector
{
public:
    
    my_vector() = default;

    my_vector(size_t count)
        : begin_(std::allocator_traits<Allocator>::allocate(allocator_, count))
        , end_(begin_ + count)
        , end_of_storage_(begin_ + count * 2)
    {
        for (size_t i = 0; i < count; ++i)
        {
            std::allocator_traits<Allocator>::construct(allocator_, begin_ + i);
        }
    }

    void push_back(const T& value)
    {
        emplace_back(value);
    }

    void push_back(T&& value)
    {
        emplace_back(std::move(value));
    }

    template<typename ...Args>
    void emplace_back(Args&&... args)
    {
        if (size() == capacity())
        {
            size_t new_capacity = capacity() * 2;
            T* new_buffer = std::allocator_traits<Allocator>::allocate(allocator_, new_capacity);

            if (new_buffer)
            {
                size_t constructed_count = 0;

                try
                {
                    for (size_t i = 0; i < size(); ++i)
                    {
                        std::allocator_traits<Allocator>::construct(allocator_, new_buffer + i, std::move_if_noexcept(begin_[i]));
                        constructed_count++;
                    }

                    std::allocator_traits<Allocator>::construct(allocator_, new_buffer + size(), std::forward<Args>(args)...);
                    constructed_count++;

                    for (size_t i = 0; i < size(); ++i)
                    {
                        std::allocator_traits<Allocator>::destroy(allocator_, begin_ + i);
                    }

                    std::allocator_traits<Allocator>::deallocate(allocator_, begin_, capacity());

                    begin_ = new_buffer;
                    end_ = begin_ + constructed_count;
                    end_of_storage_ = begin_ + new_capacity;
                } 
                catch (...)
                {
                    for (size_t i = 0; i < constructed_count; ++i)
                    {
                        std::allocator_traits<Allocator>::destroy(allocator_, new_buffer + i);
                    }

                    std::allocator_traits<Allocator>::deallocate(allocator_, new_buffer, new_capacity);

                    throw ;
                }
            }
        }
        else
        {
            std::allocator_traits<Allocator>::construct(allocator_, end_, std::forward<Args>(args)...);
            end_++;
        }
    }

    void reserve(const size_t new_capacity)
    {
        if (new_capacity > capacity())
        {
            reallocate(new_capacity);
        }
    }

    size_t capacity() const
    {
        return end_of_storage_ - begin_;
    }

    size_t size() const
    {
        return end_ - begin_;
    }

    const T& operator[](size_t index) const
    {
        return begin_[index];
    }

    ~my_vector()
    {
        for (size_t i = 0; i < size(); ++i)
        {
            std::allocator_traits<Allocator>::destroy(allocator_, begin_ + i);
        }

        std::allocator_traits<Allocator>::deallocate(allocator_, begin_, capacity());
    }

private:

    void reallocate(const size_t new_capacity)
    {
        size_t old_capacity = capacity();
        size_t old_size = size();

        T* new_begin = std::allocator_traits<Allocator>::allocate(allocator_, new_capacity);
        
        for (size_t i = 0; i < old_size; ++i)
        {
            std::allocator_traits<Allocator>::construct(allocator_, new_begin + i, std::move_if_noexcept(begin_[i]));
        }

        for (size_t i = 0; i < old_size; ++i)
        {
            std::allocator_traits<Allocator>::destroy(allocator_, begin_ + i);
        }

        std::allocator_traits<Allocator>::deallocate(allocator_, begin_, old_capacity);

        begin_ = new_begin;
        end_ = new_begin + old_size;
        end_of_storage_ = new_begin + new_capacity;
    }

private:
    [[no_unique_address]] Allocator allocator_;

    T* begin_ = nullptr;
    T* end_ = nullptr;
    T* end_of_storage_ = nullptr;
};

#define THROW_CTOR true

class A {
    public:
    A()
    {
        std::cout << "A" << std::endl;
    }

    A(const A& copyctor) noexcept(!THROW_CTOR){

        std::cout << "A(const A&)" << std::endl;

        if constexpr(THROW_CTOR)
        {
            throw std::runtime_error("some exception");
        }

    }

    A(A&& movector) noexcept(!THROW_CTOR)
    {
        std::cout << "A(A&&)" << std::endl;

        if constexpr(THROW_CTOR)
        {
            throw std::runtime_error("some exception");
        }
    }

    ~A() {
        std::cout << "~A" << std::endl;
    }
};

int main()
{
    // std::vector<A> vc;
    my_vector<A> vc;

    vc.reserve(5);
    for (int i = 0; i < 5; i++)
    {
        vc.emplace_back();
    }

    try
    {
        std::cout << "size before throw: " << vc.size() << ", capacity before throw: " << vc.capacity() << std::endl;

        vc.push_back(A());
    } catch (std::exception&)
    {
        std::cout << "size after throw: " << vc.size() << ", capacity after throw: " << vc.capacity() << std::endl;
    }

    return 0;
}