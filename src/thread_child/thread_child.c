#include "thread_child.h"
#include "head.h"
#include "main.h"
#include "thread_main.h"

void *thread_child_handle(void *args) {
    int ret = 0;
    struct thread_resource_t *thread_resource = (struct thread_resource_t *)args;

    // 初始化工作环境
    // 先上锁再进循环
    pthread_mutex_lock(&thread_resource->mutex);
    while (1) {
        // 尝试出队, 注意此时锁处于上锁状态
        struct queue_elem_t elem;
        ret = queue_out(thread_resource->queue, &elem);
        if (-1 == ret) {
            // 出队失败, 此时队列为空, 则在条件变量上等待
            pthread_cond_wait(&thread_resource->cond, &thread_resource->mutex);
        } else {
            // 出队成功, 拿到传输的对端所连接的 socket 文件描述符
            // 首先执行解锁操作
            pthread_mutex_unlock(&thread_resource->mutex);
            sprintf(logbuf, "子线程 %ld 已唤醒, 开始执行与 %d 号连接进行 MD5 校验码为 %s 的文件传输的任务.", pthread_self(), elem.connect_fd, elem.file_md5);
            logging(LOG_INFO, logbuf);

            // 情况 QUEUE_FLAG_S2C: 执行 s2c 任务
            if (QUEUE_FLAG_S2C == elem.flag) {
                // 打开文件
                char filedir[1024] = {0};
                strcat(filedir, thread_resource->filepool_dir);
                strcat(filedir, elem.file_md5);
                int filefd = open(filedir, O_RDONLY);
                // 执行文件传输
                ret = child_s2c(elem.connect_fd, filefd, elem.filesize);
                if (-1 == ret) {
                    sprintf(logbuf, "与 %d 号连接之间进行的 s2c 任务失败.", elem.connect_fd);
                } else {
                    sprintf(logbuf, "成功完成与 %d 号连接之间的 s2c 任务.", elem.connect_fd);
                }
                logging(LOG_INFO, logbuf);
                // 关闭文件
                close(filefd);
                // 通知主线程
                write(thread_resource->pipe_fd[1], &elem.connect_fd, sizeof(elem.connect_fd));
            }

            // 情况 QUEUE_FLAG_C2S: 执行 c2s 任务
            if (QUEUE_FLAG_C2S == elem.flag) {
                // code here
            }

            // 上锁, 并继续循环尝试出队.
            pthread_mutex_lock(&thread_resource->mutex);
        }
        // 注意: 尝试出队失败则 wait, 尝试出队成功则执行任务. 只有在 wait 的上下半之间(阻塞过程中), 以及执行任务期间, 锁处于解锁状态, 其他时间锁一律处于上锁状态.
    }

    return NULL;
}