/*
Написать свой мьютекс на CAS. Написать пару тестов, прогнать в Relacy и tsan.
*/

/* 
Relacy:
    clang++ 1_cas_mutex.cpp -std=c++23 -O0 -DUSE_RELACY
    ./a.out

TSan:
    clang++ 1_cas_mutex.cpp -std=c++23 -O1 -g -fsanitize=thread -pthread
    ./a.out
*/ 

#ifdef USE_RELACY
#include <relacy/relacy_std.hpp>
#else
    #include <atomic>
    #include <thread>
    #include <vector>
#endif

class cas_mutex
{
public:
    void lock()
    {
        bool expected = false;
        bool desired = true;
        while(!is_locked.compare_exchange_weak(expected, desired, std::memory_order_acquire, std::memory_order_relaxed))
        {
            expected = false;
        }
    }

    void unlock()
    {
        is_locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> is_locked = false;
};

#ifdef USE_RELACY
struct cas_mutex_test : rl::test_suite<cas_mutex_test, 2> 
{
    cas_mutex mutex;
    rl::var<int> data;
    void before() 
    {
        data($) = 0;
    }

    void thread(unsigned thread_index)
    {
        if (thread_index == 0)
        {
            mutex.lock();
            data($) = 1234;
            mutex.unlock();
        }
        else if (thread_index == 1)
        {
            int other_data = 0;

            mutex.lock();
            other_data = data($) + 1;
            data($) = 4321;
            mutex.unlock();
        }
    }
};
#endif

int main()
{
#ifdef USE_RELACY
    rl::test_params params;
    params.iteration_count = 100000;
    rl::simulate<cas_mutex_test>();
#else

    cas_mutex mutex;
    int data = 0;

    auto worker = [&]()
    {
        for (int i = 0; i < 10000; ++i)
        {
            mutex.lock();
            ++data;
            mutex.unlock();
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();
#endif
    return 0;
}