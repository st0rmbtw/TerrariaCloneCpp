#ifndef UTILS_CONCURRENT_LOOP_HPP_
#define UTILS_CONCURRENT_LOOP_HPP_

#include <functional>
#include <thread>

inline constexpr unsigned MAX_THREAD_COUNT = static_cast<unsigned>(-1);
inline constexpr unsigned MAX_THREAD_COUNT_STATIC_ARRAY = 64;

inline void DoConcurrentRangeInWorkerContainer(
    const std::function<void(std::size_t begin, std::size_t end)>&  task,
    std::size_t                                                     count,
    std::size_t                                                     workerCount,
    std::thread*                                                    workers
) {
    /* Distribute work to threads */
    const std::size_t workSize          = count / workerCount;
    const std::size_t workSizeRemain    = count % workerCount;

    std::size_t offset = 0;

    for (size_t i = 0; i < workerCount; ++i)
    {
        workers[i] = std::thread(task, offset, offset + workSize);
        offset += workSize;
    }

    /* Execute task of remaining work on main thread */
    if (workSizeRemain > 0)
        task(offset, offset + workSizeRemain);

    /* Join worker threads */
    for (size_t i = 0; i < workerCount; ++i) workers[i].join();
}

inline unsigned Log2Uint(unsigned n)
{
    unsigned nLog2 = 0;
    while (n >>= 1)
        ++nLog2;
    return nLog2;
}

inline unsigned ClampThreadCount(unsigned threadCount, std::size_t workSize, unsigned threadMinWorkSize)
{
    if (workSize > threadMinWorkSize)
    {
        if (threadCount == MAX_THREAD_COUNT)
        {
            /* Compute number of threads automatically logarithmically to the workload */
            threadCount = Log2Uint(static_cast<unsigned>(workSize / threadMinWorkSize));

            /*
            Clamp to maximum number of threads support by the CPU.
            If this value is undefined or not comutable, the return value of the STL function is 0.
            */
            const unsigned maxThreadCount = std::thread::hardware_concurrency();
            if (maxThreadCount > 0)
                threadCount = std::min(threadCount, maxThreadCount);
        }

        /* Clamp final number of threads by the minimum workload per thread */
        return std::min(threadCount, static_cast<unsigned>(workSize / threadMinWorkSize));
    }
    return 0;
}

inline void DoConcurrentRange(
    std::size_t                                                     count,
    const std::function<void(std::size_t begin, std::size_t end)>&  task,
    unsigned                                                        threadCount,
    unsigned                                                        threadMinWorkSize
) {
    threadCount = ClampThreadCount(threadCount, count, threadMinWorkSize);

    if (threadCount <= 1)
    {
        /* Run single-threaded */
        task(0, count);
    }
    else if (threadCount <= MAX_THREAD_COUNT_STATIC_ARRAY)
    {
        /* Launch worker threads in static array */
        std::thread workers[MAX_THREAD_COUNT_STATIC_ARRAY];
        DoConcurrentRangeInWorkerContainer(task, count, threadCount, workers);
    }
    else if (threadCount > 1)
    {
        /* Launch worker threads in dynamic array */
        std::vector<std::thread> workers(threadCount);
        DoConcurrentRangeInWorkerContainer(task, count, threadCount, workers.data());
    }
}

inline void DoConcurrent(
    std::size_t                                     count,
    const std::function<void(std::size_t index)>&   task,
    unsigned                                        threadCount = static_cast<uint32_t>(-1),
    unsigned                                        threadMinWorkSize = 64
) {
    DoConcurrentRange(
        count,
        [&task](std::size_t begin, std::size_t end)
        {
            for (size_t i = begin; i < end; ++i)
                task(i);
        },
        threadCount,
        threadMinWorkSize
    );
}

#endif