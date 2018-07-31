//
// Created by yang on 7/31/18.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H

#include "Locker.h"
#include <list>

template <typename  T>
class ThreadPool {
public:ThreadPool(int thread_number=8,int max_requests=10000);
    ~ThreadPool();
    bool append(T *request);

private:
    static void * worker(void *arg);
    void run();

private:
    int m_thread_number;
    int m_max_requests;
    pthread_t* m_threads;
    std::list< T* > m_workqueue;
    locker m_queuelocker;
    sem m_queuestat;
    bool m_stop;
};



#endif //THREADPOOL_THREADPOOL_H
