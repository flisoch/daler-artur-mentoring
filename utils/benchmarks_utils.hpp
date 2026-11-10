#pragma once

void clean_up_cpu_cache()
{
    // my L3 cache is 12 MiB, so I allocate 36 MiB of data and iterate through it
    // int64_t = 8 bytes, 25 MiB = 36 * 1024 * 1024 bytes = ~ 4.5 million elements + 1 (LLM said to add +1)

    std::vector<int64_t> garbage(36 * 1024 * 128 + 1, 0xDEADBEEF);

    // L3 critical stride is 768 KB. If I hit only one cache set, It gives pollution to only 1 cache set. So I add +64 bytes to my Critical Stride to hit next cache set on every iteration
    // (768 * 1024 + 64) / 8 = 98312
    volatile int64_t sum = 0;
    size_t current_index = 0;

    for (size_t total_reads = 0; total_reads < garbage.size(); ++total_reads)
    {
        sum += garbage[current_index];
        current_index = (current_index + 98312) % garbage.size();
    }

    asm volatile("": "+r" (sum));

    /*
    $ perf stat -e LLC-loads,LLC-load-misses,page-faults ./a.out

    Performance counter stats for './a.out':

         4,402,288      LLC-loads                                                             
         3,771,147      LLC-load-misses        // ~85%                                    
             9,355      page-faults            // 9216 pages need to allocate 36 MiB vector if page=4kb
    */
}

