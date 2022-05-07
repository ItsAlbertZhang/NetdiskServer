#include "head.h"
#include "program_stat.h"
#include "thread_main.h"

static int connect_init(int socket_fd, struct connect_stat_t *connect_stat_arr, int max_connect_num, struct connect_timer_hashnode *connect_timer_arr);

int epoll_handle_socket(int socket_fd, struct connect_stat_t *connect_stat_arr, int max_connect_num, struct connect_timer_hashnode *connect_timer_arr) {
    int ret = 0;

    // 初始化连接
    ret = connect_init(socket_fd, connect_stat_arr, max_connect_num, connect_timer_arr);
    RET_CHECK_BLACKLIST(-1, ret, "connect_init");

    return 0;
}

static int connect_init(int socket_fd, struct connect_stat_t *connect_stat_arr, int max_connect_num, struct connect_timer_hashnode *connect_timer_arr) {
    int ret = 0;

    // accept 连接并保存连接状态
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t addrlen = sizeof(addr);
    int connect_fd = accept(socket_fd, (struct sockaddr *)&addr, &addrlen); // accept 连接
    RET_CHECK_BLACKLIST(-1, connect_fd, "accept");

    // 保存连接状态
    int i = connect_fd % max_connect_num; // 新连接应该在的数组下标, 此位置应为空.
    if (connect_stat_arr[i].fd) {
        return -1; // 如果此处已有连接, 则返回 -1. (DEBUG: 这里还是有一点问题的)
    }
    connect_stat_arr[i].fd = connect_fd;
    memcpy(&connect_stat_arr[i].addr, &addr, sizeof(addr));

    // 为新连接设置连接计时器
    ret = connect_timer_in(&connect_stat_arr[i], connect_timer_arr, NULL);
    RET_CHECK_BLACKLIST(-1, ret, "connect_timer_in");
    printf("new connect fd = %d\n", connect_stat_arr[i].fd);

    return 0;
}
