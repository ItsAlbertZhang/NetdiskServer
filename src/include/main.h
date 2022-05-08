#ifndef __MAIN_H__
#define __MAIN_H__

#include "head.h"

// 子线程共用资源
struct thread_resource_t {
    struct queue_t *queue; // 任务队列
    pthread_mutex_t mutex; // 线程锁, 加解锁后方可对队列进行操作
    pthread_cond_t cond;   // 子线程等待所在的条件变量
};

// 线程池状态
struct thread_stat_t {
    pthread_t *pthid;
    int pth_num;
    struct thread_resource_t thread_resource;
};

// 进程状态
struct program_stat_t {
    RSA *private_rsa;
    RSA *public_rsa;
    MYSQL *mysql_connect;
    struct thread_stat_t thread_stat;
    int socket_fd;
    char local_ip[16];
};

int program_init(struct program_stat_t *program_stat);

int thread_main_handle(struct program_stat_t *program_stat);

#endif /* __MAIN_H__ */