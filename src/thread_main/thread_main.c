#include "thread_main.h"
#include "head.h"
#include "mylibrary.h"
#include "program_stat.h"

static int epoll_add(int epfd, int fd);

int thread_main_handle(struct program_stat_t *program_stat) {
    int ret = 0;

    // 最大连接数: 相当于 子线程个数 + 等待子线程服务的队列容量.
    int max_connect_num = program_stat->thread_stat.pth_num + program_stat->thread_stat.thread_resource.queue->len;
    // 申请一块内存, 用于存放连接状态. 可使用连接的文件描述符快速找到连接对应的状态.
    // 借由文件描述符是从小到大的顺序的特性, 可以将每个连接的状态放置在 (连接文件描述符 % 最大连接数) 为下标的位置.
    struct connect_stat_t *connect_stat_arr = (struct connect_stat_t *)malloc(sizeof(struct connect_stat_t) * max_connect_num);
    bzero(connect_stat_arr, sizeof(struct connect_stat_t) * max_connect_num); // 清空连接状态

    // 连接计时器初始化
    struct connect_timer_hashnode connect_timer_arr[AUTO_DISCONNECT_SECOND]; // 每 AUTO_DISCONNECT_SECOND 秒一轮
    bzero(connect_timer_arr, sizeof(struct connect_timer_hashnode) * AUTO_DISCONNECT_SECOND);

    // epoll 初始化
    int epfd = epoll_create(1);                     // 创建 epoll 句柄
    ret = epoll_add(epfd, program_stat->socket_fd); // 将 socket_fd 添加至 epoll 监听
    RET_CHECK_BLACKLIST(-1, ret, "epoll_add");

    // 定义一个返回的监听结果的数组. 至多同时监听 (最大连接数 + 1 个 socket) 个目标.
    struct epoll_event *events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * (max_connect_num + 1));
    bzero(events, sizeof(struct epoll_event) * (max_connect_num + 1));
    int ep_ready = 0; // 有消息来流的监听个数

    char program_running_flag = 1; // 程序继续运行标志
    while (program_running_flag) {
        ep_ready = epoll_wait(epfd, events, (max_connect_num + 1), 1000); // 进行 epoll 多路监听, 至多监听 1 秒.
        RET_CHECK_BLACKLIST(-1, ep_ready, "epoll_wait");
        for (int i = 0; i < ep_ready; i++) {
            if (events[i].data.fd == program_stat->socket_fd) { // 消息来源为 socket_fd, 有新连接
                ret = epoll_handle_socket(program_stat->socket_fd, connect_stat_arr, max_connect_num, connect_timer_arr);
                RET_CHECK_BLACKLIST(-1, ret, "epoll_handle_socket");
            } else { //  消息来源为已有连接
                // ret = epoll_handle_established();
            }
        }
        ret = connect_timer_handle_next_second(connect_timer_arr);
        RET_CHECK_BLACKLIST(-1, ret, "connect_timer_handle_next_second");
    }
}

static int epoll_add(int epfd, int fd) {
    static int ret = 0;
    static struct epoll_event event;

    bzero(&event, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = fd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    RET_CHECK_BLACKLIST(-1, ret, "epoll_ctl");

    return 0;
}