// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/8/13
// Description: 分别使用信号量和条件变量实现生产者消费者问题

#include "14_2.h"
#include <ctime>
#include <cstdlib>
#include <vector>

using namespace synchronize;
using std::to_string;

namespace impl_sem {

    template <typename T>
    class Pool {
        T *p;
        int front, tail;
        int size;
        int capacity;
        Sem sem_empty, sem_full;
        Mutex mutex;
        Log log;
        void backward(int &x) {
            x = (x + 1) % capacity;
        }
    public:
        explicit Pool(int _capacity = 5)
        : capacity(_capacity)
        , front(0), tail(0), size(0)
        , sem_empty(_capacity)
        , sem_full(0)
        , log("../Log/sync_sem.log") {
            p = new T[capacity];
        }
        ~Pool() {
            delete[] p;
        }
        Pool(const Pool&) = delete;
        Pool & operator = (const Pool &) = delete;
        void produce(const T& tmp, const std::string &msg = "") {
            sem_empty.wait();
            //进入临界区
            mutex.lock();
            p[tail] = tmp;
            backward(tail);
            //        log.print(to_string(tail) + " " + to_string(tmp) + " produced");
            log.print(msg + tmp);
            ++size;
            mutex.unlock();
            sem_full.post();
        }
        T consume(const std::string &msg = "") {
            sem_full.wait();
            //进入临界区
            mutex.lock();
            T ret = p[front];
            backward(front);
            //        log.print(to_string(front) + " " + to_string(ret) + " consumed");
            log.print( msg + ret);
            --size;
            mutex.unlock();
            sem_empty.post();
            return ret;
        }
    };
}

namespace impl_cond {

    template <typename T>
    class Pool {
        T *p;
        int front, tail;
        int size;
        int capacity;
        Mutex mutex;
        Cond cond_empty, cond_full;
        Log log;
        void backward(int &x) {
            x = (x + 1) % capacity;
        }
    public:
        explicit Pool(int _capacity = 5)
                : capacity(_capacity)
                , front(0), tail(0), size(0)
                , mutex()
                , cond_empty(&mutex), cond_full(&mutex)
                , log("../Log/sync_cond.log") {
            p = new T[capacity];
        }
        ~Pool() {
            delete[] p;
        }
        Pool(const Pool&) = delete;
        Pool & operator = (const Pool &) = delete;
        void produce(const T& tmp, const std::string &msg = "") {
            cond_empty.lock();
            while (size == capacity) {
                cond_empty.wait();
            }
            //进入临界区
            p[tail] = tmp;
            backward(tail);
            ++size;
            log.print(to_string(size) + " " + msg + " " + tmp);
            //log.print(msg + " " + tmp);
            cond_empty.unlock();
            cond_full.signal();
        }
        T consume(const std::string &msg = "") {
            cond_full.lock();
            while (size == 0) {
                cond_full.wait();
            }
            //进入临界区
            T ret = p[front];
            backward(front);
            --size;
            log.print(to_string(size) + " " + msg + " " + ret);
            //log.print( msg + " " + ret);
            cond_full.unlock();
            cond_empty.signal();
            return ret;
        }
    };

}

using Pool = impl_sem::Pool<std::string>;

void * produce(void *_p) {
    auto p = reinterpret_cast<std::pair<std::string, Pool *> *>(_p);
    const std::string msg = p->first;
    Pool *pool = p->second;
    delete p;
    int i = 0;
    while (true) {
        pool->produce("[" + msg + ":" + to_string(++i) + "]", msg);
//        sleep(rand() % 3);
    }
}

void * consume(void *_p) {
    auto p = reinterpret_cast<std::pair<std::string, Pool *> *>(_p);
    const std::string msg = p->first;
    Pool *pool = p->second;
    delete p;
    while (true) {
        pool->consume(msg);
//        sleep(rand() % 3);
    }
}

void producer_create(std::vector<pthread_t> &tid, int n, Pool *pool) {
    for (int i = 1; i <= n; ++i) {
        pthread_t t;
        pthread_create(&t, nullptr, produce, new std::pair<std::string, Pool *>{"producer" + to_string(i), pool});
        tid.push_back(t);
    }
}

void consumer_create(std::vector<pthread_t> &tid, int n, Pool *pool) {
    for (int i = 1; i <= n; ++i) {
        pthread_t t;
        pthread_create(&t, nullptr, consume, new std::pair<std::string, Pool *>{"consumer" + to_string(i), pool});
        tid.push_back(t);
    }
}

/*!
 * 生产者消费者模型
 * @param capacity 生产池的大小
 * @param producer_n 生产者个数
 * @param consumer_n 消费者个数
 */
void model(int capacity = 5, int producer_n = 3, int consumer_n = 3) {
    Pool *pool = new Pool(capacity);
    srand(time(nullptr));

    std::vector<pthread_t> tid;
    producer_create(tid, producer_n, pool);
    consumer_create(tid, consumer_n, pool);

    sleep(1);

    for (auto t : tid) {
        pthread_cancel(t);
    }

    delete pool;
}

int main() {
    model();
    return 0;
}