#include "child_thread.h"
#include "head.h"
#include "mylibrary.h"
#include "program_init.h"
#include "program_stat.h"

int init_pthread_pool(struct thread_stat_t *thread_stat, const char *config_dir, char config[][MAX_CONFIG_LENGTH]) {
    int ret = 0;

    // 获取线程池配置
    ret = getconfig(config_dir, "pthread.config", config);
    RET_CHECK_BLACKLIST(-1, ret, "getconfig");

    // 初始化线程状态
    thread_stat->pthid = (pthread_t *)malloc(sizeof(pthread_t) * atoi(config[0])); // 初始化线程 id 数组
    // 初始化线程资源
    queue_init(&thread_stat->thread_resource.queue, atoi(config[1])); // 初始化任务队列
    pthread_mutex_init(&thread_stat->thread_resource.mutex, NULL);    // 初始化线程锁
    pthread_cond_init(&thread_stat->thread_resource.cond, NULL);      // 初始化条件变量

    // 拉起子线程
    for (int i = 0; i < atoi(config[0]); i++) {
        ret = pthread_create(&thread_stat->pthid[i], NULL, thread_child_handle, (void *)&thread_stat->thread_resource);
        THREAD_RET_CHECK(ret, "pthread_create");
    }

    return 0;
}