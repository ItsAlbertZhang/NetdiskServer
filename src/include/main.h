#ifndef __MAIN_H__
#define __MAIN_H__

#include "head.h"
#include "mylibrary.h"

// 子线程共用资源
struct thread_resource_t {
    struct queue_t *queue;   // 任务队列 (主线程结合任务队列与条件变量向子线程发送消息)
    int pipe_fd[2];          // 管道 (子线程通过管道向主线程发送消息)
    pthread_mutex_t mutex;   // 线程锁, 加解锁后方可对队列进行操作
    pthread_cond_t cond;     // 子线程等待所在的条件变量
    char filepool_dir[1024]; // 文件池所在目录
};

// 线程池状态
struct thread_stat_t {
    pthread_t *pthid;    // 数组, 用于保存子线程 id
    int pth_num;         // 子线程个数
    /*  最大同时连接数. 其值应为: 子线程个数 * 2 + 等待子线程服务的队列最大长度.
        
        准确而言, 应当称其为 "最大同时活动文件数" .
        
        原因: 连接状态 struct connect_stat_t 储存在 struct connect_stat_t * 数组中. 
        
        为了在 epoll 监听中快速定位消息来流连接, 连接状态使用了其 socket 文件描述符对最大同时活动文件数取余作为其在连接状态数组中的下标.
        
        而与此同时, 子线程在进行任务传输时需要打开本地文件.
        
        将连接所使用的 socket 文件 与 本地文件 合称为 "活动文件". 显然, 在这些活动文件中, 只有 socket 文件对应的是真正的连接.
        
        因此, 真正的 "最大同时连接数" 是一个活动的范围.
        
        以 5 个子线程, 等待子线程服务队列长度为 10 的情况举例. 其最大同时活动文件数为 20. 在所有子线程都不工作的情况下, 其可容纳 20 个连接, 第 21 个连接会由于落在第 1 个连接所在位置而被拒绝. 但在所有子线程都打开文件工作的情况下, 其仅可容纳 15 个连接, 第 16 个连接便会由于落在第 1 个连接所在位置而被拒绝.
    */
    int max_connect_num; 
    struct thread_resource_t thread_resource;
};

// 进程状态
struct program_stat_t {
    RSA *private_rsa;
    RSA *public_rsa;
    MYSQL *mysql_connect;
    struct thread_stat_t thread_stat;
    int socket_fd;
    char local_sign[64]; // 用于在 MySQL 数据库中的日志签名
};

enum log_type {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
};

int program_init(struct program_stat_t *program_stat);

int thread_main_handle(struct program_stat_t *program_stat);

#endif /* __MAIN_H__ */