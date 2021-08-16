// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/8/9
// Description: 多线程死锁

#include <pthread.h>
#include <iostream>
#include <sys/time.h>
#include <signal.h>
#include <cstring>
#include <cstdio>
#include <semaphore.h>

using namespace std;

namespace {
    pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
    sem_t sem1, sem2;
}

void alarm_handler(int signo) {

}

struct timeval my_sleep(struct timeval it_value) {
    //捕获SIGALRM信号
    struct sigaction new_act, old_act;
    memset(&new_act, 0, sizeof(new_act));
    new_act.sa_flags |= SA_RESTART;
    sigfillset(&new_act.sa_mask);
    new_act.sa_handler = alarm_handler;
    sigaction(SIGALRM, &new_act, &old_act);

    //屏蔽SIGALRM信号
    sigset_t new_set, old_set;
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &new_set, &old_set);

    //开始计时
    struct itimerval new_itimer, old_itimer;
    new_itimer.it_value = it_value;
    new_itimer.it_interval = {0, 0};
    setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

    //原本是将new_set设置为全1，看了一下以前的代码发现应该将new_set设为old_set，这样就不会把不该屏蔽的屏蔽了
    new_set = old_set;
    sigdelset(&new_set, SIGALRM);
    //主动阻塞挂起等待信号
    sigsuspend(&new_set);

    //取消定时
    getitimer(ITIMER_REAL, &new_itimer);
    setitimer(ITIMER_REAL, &old_itimer, nullptr);

    //恢复阻塞信号集/信号屏蔽字，解除对SIGALRM的屏蔽
    sigprocmask(SIG_SETMASK, &old_set, nullptr);

    //取消捕捉SIGALRM信号
    sigaction(SIGALRM, &old_act, nullptr);
    return new_itimer.it_value;
}

void *task_a(void *) {
//    pthread_mutex_lock(&mutex1);
    sem_wait(&sem1);
    printf("A have locked sem1\n");
    my_sleep({3, 0});
//    pthread_mutex_lock(&mutex2);
    sem_wait(&sem2);
    printf("A have locked sem2\n");
//    pthread_mutex_unlock(&mutex2);
//    pthread_mutex_unlock(&mutex1);
    sem_post(&sem2);
    sem_post(&sem1);
}


void *task_b(void *) {
//    pthread_mutex_lock(&mutex2);
    sem_wait(&sem2);
    printf("B have locked sem2\n");
    my_sleep({3, 0});
//    pthread_mutex_lock(&mutex1);
    sem_wait(&sem1);
    printf("B have locked sem1\n");
//    pthread_mutex_unlock(&mutex1);
//    pthread_mutex_unlock(&mutex2);
    sem_post(&sem1);
    sem_post(&sem2);
}

int main() {
//    for (int i = 0; i < 10; ++i) {
//        my_sleep({1, 0});
//        printf("hello\n");
//    }
    sem_init(&sem1, PTHREAD_PROCESS_SHARED, 1);
    sem_init(&sem2, PTHREAD_PROCESS_SHARED, 1);
    pthread_t tid_a, tid_b;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
//    设置游离态，不需要回收
//    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid_a, &attr, task_a, nullptr);
    pthread_create(&tid_b, &attr, task_b, nullptr);

    pthread_join(tid_a, nullptr);
    pthread_join(tid_b, nullptr);


    sem_destroy(&sem1);
    sem_destroy(&sem2);
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    return 0;
}