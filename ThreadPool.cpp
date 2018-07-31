//
// Created by yang on 7/31/18.
//

#include <cstdio>
#include "ThreadPool.h"


template<typename T>
ThreadPool<T>::ThreadPool(int thread_number, int max_requests)
        :m_thread_number(thread_number)
        ,m_max_requests(max_requests)
        , m_threads(nullptr) {
    if ((thread_number <= 0) || (max_requests <= 0)) {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }

    for (int i = 0; i < thread_number; ++i) {
        printf("create the %dth thread\n", i);
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    delete[] m_threads;
    m_stop = true;
}


template<typename T>
bool ThreadPool<T>::append(T *request) {
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
void *ThreadPool<T>::worker(void *arg) {
    ThreadPool *pool = (ThreadPool *) arg;
    pool->run();
    return pool;
}

template<typename T>
void ThreadPool<T>::run() {
    printf("first %d\n",m_stop);
    while (!m_stop) {
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }
        request->process();
    }
}
