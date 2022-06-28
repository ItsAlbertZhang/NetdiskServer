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
        struct thread_task_queue_elem_t thread_task_queue_elem;
        ret = queue_out(thread_resource->task_queue, &thread_task_queue_elem);
        if (-1 == ret) {
            // 出队失败, 此时队列为空, 则在条件变量上等待
            pthread_cond_wait(&thread_resource->cond, &thread_resource->mutex);
        } else {
            // 出队成功, 拿到传输的对端所连接的 socket 文件描述符

            // 在解锁之前, 对互斥资源进行操作
            // 获取一个独占资源, 用于执行本次任务
            struct thread_exclusive_resources_queue_elem_t exclusive_resource_queue_elem;
            queue_out(thread_resource->exclusive_resources_queue, &exclusive_resource_queue_elem);
            // 将独占资源的进度条的指针填入进度条队列, 以方便在主进程访问进度
            struct progress_t *progress_bar_p = &exclusive_resource_queue_elem.progress_bar;
            queue_in(thread_resource->progress_queue, &progress_bar_p);
            // queue_in(thread_resource->progress_queue, &exclusive_resource_queue_elem.progress_bar);

            // 执行解锁操作
            pthread_mutex_unlock(&thread_resource->mutex);
            // sprintf(logbuf, "子线程 %ld 已唤醒, 开始执行与 %d 号连接进行 MD5 校验码为 %s 的文件传输的任务.", pthread_self(), thread_task_queue_elem.connect_fd, thread_task_queue_elem.file_md5);
            // logging(LOG_INFO, logbuf);

            // 初始化进度条
            progress_bar_p->filesize = thread_task_queue_elem.filesize;
            progress_bar_p->completedsize = 0;
            progress_bar_p->lastsize = 0;
            progress_bar_p->starttime = time(NULL);
            strcpy(progress_bar_p->file_md5, thread_task_queue_elem.file_md5);

            // 情况 QUEUE_FLAG_S2C: 执行 s2c 任务
            if (QUEUE_FLAG_S2C == thread_task_queue_elem.flag) {
                // 打开文件
                char filedir[1024] = {0};
                strcat(filedir, thread_resource->filepool_dir);
                strcat(filedir, thread_task_queue_elem.file_md5);
                int filefd = open(filedir, O_RDONLY);
                // 执行文件传输
                ret = child_s2c(thread_task_queue_elem.connect_fd, filefd, &exclusive_resource_queue_elem.progress_bar);
                // 关闭文件
                close(filefd);
                // 通知主线程
                write(thread_resource->pipe_fd[1], &thread_task_queue_elem.connect_fd, sizeof(thread_task_queue_elem.connect_fd));
            }

            // 情况 QUEUE_FLAG_C2S: 执行 c2s 任务
            if (QUEUE_FLAG_C2S == thread_task_queue_elem.flag) {
                // code here
            }

            // 等待 1 秒, 以使可能的进度条显示完成
            // sleep(1);

            // 上锁并释放独占资源, 之后继续循环尝试出队.
            pthread_mutex_lock(&thread_resource->mutex);
            struct progress_t *temp = NULL;
            queue_out(thread_resource->progress_queue, &temp);
            while (temp != progress_bar_p) {
                queue_in(thread_resource->progress_queue, &temp);
                queue_out(thread_resource->progress_queue, &temp);
            }
            queue_in(thread_resource->exclusive_resources_queue, &exclusive_resource_queue_elem);

            // // 上锁, 并继续循环尝试出队.
            // pthread_mutex_lock(&thread_resource->mutex);
        }
        // 注意: 尝试出队失败则 wait, 尝试出队成功则执行任务. 只有在 wait 的上下半之间(阻塞过程中), 以及执行任务期间, 锁处于解锁状态, 其他时间锁一律处于上锁状态.
    }

    return NULL;
}