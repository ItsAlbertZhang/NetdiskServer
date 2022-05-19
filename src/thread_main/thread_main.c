#include "thread_main.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"

int epfd = -1;

int thread_main_handle(struct program_stat_t *program_stat) {
    int ret = 0;

    // 最大连接数: 相当于 子线程个数 + 等待子线程服务的队列容量.
    int max_connect_num = program_stat->thread_stat.pth_num + program_stat->thread_stat.thread_resource.queue->len;
    // malloc 结构体数组: 连接状态 struct connect_stat_t; 数组的成员个数即是最大连接数.
    // 借由文件描述符是从小到大的顺序的特性, 可以将每个连接的状态放置在 (连接文件描述符 % 最大连接数) 为下标的位置.
    struct connect_stat_t *connect_stat_arr = (struct connect_stat_t *)malloc(sizeof(struct connect_stat_t) * max_connect_num);
    bzero(connect_stat_arr, sizeof(struct connect_stat_t) * max_connect_num); // 清空连接状态

    // 休眠连接链表
    struct connect_sleep_node *connect_sleep = connect_sleep_init();

    // malloc 结构体数组: 时间轮定时器 struct connect_timer_hashnode; 每个数组成员均为一个时间轮片, 数组成员个数即为时间轮一轮循环的秒数.
    // 数组成员下标是时间的除留余法散列函数, 整个时间轮定时器采用拉链法哈希构建.每个数组成员实际上为哈希表拉链的头结点.
    struct connect_timer_hashnode *connect_timer_arr = (struct connect_timer_hashnode *)malloc(sizeof(struct connect_timer_hashnode) * AUTO_DISCONNECT_SECOND);
    bzero(connect_timer_arr, sizeof(struct connect_timer_hashnode) * AUTO_DISCONNECT_SECOND);

    // epoll 初始化
    epfd = epoll_create(1);                   // 创建 epoll 句柄
    ret = epoll_add(program_stat->socket_fd); // 将 socket_fd 添加至 epoll 监听
    RET_CHECK_BLACKLIST(-1, ret, "epoll_add");
    // malloc 结构体数组: 返回的监听结果 struct epoll_event; 数组的成员个数为最大连接数 + 1 (除了要监听连接之外, 还要监听 socket)
    struct epoll_event *events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * (max_connect_num + 1));
    bzero(events, sizeof(struct epoll_event) * (max_connect_num + 1));
    int ep_ready = 0; // 有消息来流的监听个数

    char program_running_flag = 1; // 程序继续运行标志
    while (program_running_flag) {
        ep_ready = epoll_wait(epfd, events, (max_connect_num + 1), 1000); // 进行 epoll 多路监听, 至多监听 1 秒.
        RET_CHECK_BLACKLIST(-1, ep_ready, "epoll_wait");
        for (int i = 0; i < ep_ready; i++) {
            if (events[i].data.fd == program_stat->socket_fd) { // 有来自 socket_fd 的消息 (新连接)
                ret = connect_init_handle(program_stat->socket_fd, connect_stat_arr, max_connect_num, connect_timer_arr);
                RET_CHECK_BLACKLIST(-1, ret, "connect_init_handle");
            } else { //  有来自已有连接的消息
                // 处理消息
                ret = connect_msg_handle(&connect_stat_arr[events[i].data.fd % max_connect_num], connect_timer_arr, program_stat, connect_sleep);
                RET_CHECK_BLACKLIST(-1, ret, "connect_msg_handle");
                // 处理时间轮定时器
                ret = connect_timer_update(&connect_stat_arr[events[i].data.fd % max_connect_num], connect_timer_arr);
            }
        }
        // 处理下一秒对应的时间轮片上的连接
        ret = connect_timer_handle(connect_timer_arr, connect_sleep);
        RET_CHECK_BLACKLIST(-1, ret, "connect_timer_handle");
    }
}