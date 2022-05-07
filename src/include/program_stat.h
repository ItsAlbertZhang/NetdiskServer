#ifndef __PROGRAM_STAT_H__
#define __PROGRAM_STAT_H__

#include "head.h"

struct thread_resource_t {
    struct queue_t *queue; // 任务队列
    pthread_mutex_t mutex; // 线程锁, 加解锁后方可对队列进行操作
    pthread_cond_t cond;   // 子线程等待所在的条件变量
};

struct thread_stat_t {
    pthread_t *pthid;
    struct thread_resource_t thread_resource;
};

struct program_stat_t {
    RSA *private_rsa;
    RSA *public_rsa;
    MYSQL *mysql_connect;
    struct thread_stat_t thread_stat;
    char local_ip[16];
};

#endif