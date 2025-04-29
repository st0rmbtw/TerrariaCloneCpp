#include "thread_pool.hpp"

void ThreadPool::start() {
    m_should_terminate = false;

    const uint32_t num_threads = std::thread::hardware_concurrency(); // Max # of threads the system supports
    for (uint32_t ii = 0; ii < num_threads; ++ii) {
        m_threads.emplace_back(&ThreadPool::loop, this);
    }
}

void ThreadPool::loop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_mutex_condition.wait(lock, [this] {
                return !m_jobs.empty() || m_should_terminate;
            });
            if (m_should_terminate) {
                return;
            }
            job = m_jobs.front();
            m_jobs.pop();
        }
        job();
    }
}

void ThreadPool::queue_job(const std::function<void()>& job) {
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_jobs.push(job);
    }
    m_mutex_condition.notify_one();
}

bool ThreadPool::busy() {
    bool poolbusy;
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        poolbusy = !m_jobs.empty();
    }
    return poolbusy;
}

void ThreadPool::join() {
    for (std::thread& active_thread : m_threads) {
        active_thread.join();
    }
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_should_terminate = true;
    }
    m_mutex_condition.notify_all();
    for (std::thread& active_thread : m_threads) {
        active_thread.join();
    }
    m_threads.clear();
}