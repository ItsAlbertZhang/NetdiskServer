#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "program_init.h"
#include "thread_child.h"

int init_pthread_pool(struct thread_stat_t *thread_stat, const char *config_dir, char config[][MAX_CONFIG_LENGTH]) {
    int ret = 0;

    // 获取线程池配置
    ret = getconfig(config_dir, "pthread.config", config);
    RET_CHECK_BLACKLIST(-1, ret, "getconfig");

    // 初始化线程状态
    thread_stat->pth_num = atoi(config[0]);
    thread_stat->max_connect_num = thread_stat->pth_num * 2 + atoi(config[1]); // 见该结构体成员的注释
    thread_stat->pthid = (pthread_t *)malloc(sizeof(pthread_t) * thread_stat->pth_num); // 初始化线程 id 数组
    // 初始化线程资源
    queue_init(&thread_stat->thread_resource.queue, sizeof(struct queue_elem_t), atoi(config[1])); // 初始化任务队列
    pipe(thread_stat->thread_resource.pipe_fd);                       // 初始化管道
    pthread_mutex_init(&thread_stat->thread_resource.mutex, NULL);    // 初始化线程锁
    pthread_cond_init(&thread_stat->thread_resource.cond, NULL);      // 初始化条件变量
    // 初始化文件池所在目录
    strncpy(thread_stat->thread_resource.filepool_dir, config_dir, strlen(config_dir) - strlen("config/"));
    strcat(thread_stat->thread_resource.filepool_dir, "file/");

    // 拉起子线程
    for (int i = 0; i < thread_stat->pth_num; i++) {
        ret = pthread_create(&thread_stat->pthid[i], NULL, thread_child_handle, (void *)&thread_stat->thread_resource);
        THREAD_RET_CHECK(ret, "pthread_create");
    }

    return 0;
}