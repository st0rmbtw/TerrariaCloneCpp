#ifndef _UTILS_THREAD_POOL_HPP_
#define _UTILS_THREAD_POOL_HPP_

#include <functional>
#include <mutex>
#include <queue>

class ThreadPool {
public:
    void start();
    void queue_job(const std::function<void()>& job);
    void stop();
    void join();
    bool busy();

private:
    void loop();

    bool m_should_terminate = false;
    std::mutex m_queue_mutex;
    std::condition_variable m_mutex_condition;
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_jobs;
};

#endif