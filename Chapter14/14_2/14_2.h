//
// Created by edward on 2021/8/13.
//

#ifndef LINUX_SERVER_14_2_H
#define LINUX_SERVER_14_2_H

#include "wrap.h"
#include "utils.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <exception>
#include <pthread.h>

namespace synchronize {

    class Sem {
        sem_t *sem;
        bool pshared;
    public:
        explicit Sem(unsigned int value = 0, bool _pshared = false):pshared(_pshared) {
            if (pshared) {
                //如果允许进程间共享，则要求信号量定义在可以在进程间共享的匿名映射区中
                void *p = mmap(nullptr, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
                if (p == MAP_FAILED) {
                    perror("mmap error");
                    exit(1);
                }
                sem = reinterpret_cast<sem_t *>(p);
            } else {
                //如果线程间共享，则定义在堆上即可
                sem = new sem_t;
            }
            //初始化
            check_error(sem_init(sem, pshared, value), "sem_init error");
        }
        ~Sem() {
            check_error(sem_destroy(sem), "sem_destroy error");
            if (pshared) {
                //定义在匿名映射区上
                check_error(munmap(sem, sizeof(sem_t)), "munmap error");
            } else {
                //定义在堆上
                delete sem;
            }
        }

        void wait() {
            check_error(sem_wait(sem), "sem_wait error");
        }
        void post() {
            check_error(sem_post(sem), "sem_post error");
        }

        sem_t * get_sem_t() const {
            return sem;
        }

        //不允许拷贝
        Sem(const Sem &) = delete;
        Sem & operator = (const Sem &) = delete;
    };

    class Mutex {
        pthread_mutex_t *mutex;
    public:
        Mutex() {
            mutex = new pthread_mutex_t;
            pthread_check_error(pthread_mutex_init(mutex, nullptr), "pthread_mutex_init error");
        }
        ~Mutex() {
            int ret = pthread_mutex_destroy(mutex);

            pthread_check_error(ret ,"pthread_mutex_destroy error");
            delete mutex;
        }
        void lock() {
            pthread_check_error(pthread_mutex_lock(mutex), "pthread_mutex_lock error");
        }
        void unlock() {
            pthread_check_error(pthread_mutex_unlock(mutex), "pthread_mutex_unlock error");
        }
        pthread_mutex_t * get_mutex_t() const {
            return mutex;
        }
        Mutex(const Mutex&) = delete;
        Mutex & operator = (const Mutex&) = delete;
    };

    class Cond {
        pthread_cond_t *cond;
        Mutex *mutex;
    public:
        explicit Cond(Mutex *_mutex) {
            cond = new pthread_cond_t;
            pthread_check_error(pthread_cond_init(cond, nullptr), "pthread_cond_init error");
            mutex = _mutex;
        }
        ~Cond() {
            pthread_check_error(pthread_cond_destroy(cond), "pthread_cond_destroy error");
            delete cond;
        }
        void lock() {
            mutex->lock();
        }
        void unlock() {
            mutex->unlock();
        }
        void wait() {
            pthread_check_error(pthread_cond_wait(cond, mutex->get_mutex_t()), "pthread_cond_wait error");
        }
        void signal() {
            pthread_check_error(pthread_cond_signal(cond), "pthread_cond_signal error");
        }
        void broadcast() {
            pthread_check_error(pthread_cond_broadcast(cond), "pthread_cond_broadcast error");
        }
        pthread_cond_t * get_cond_t() const {
            return cond;
        }
        Mutex * get_mutex() const {
            return mutex;
        }
        Cond(const Cond&) = delete;
        Cond & operator = (const Cond&) = delete;
    };
}


#endif //LINUX_SERVER_14_2_H
