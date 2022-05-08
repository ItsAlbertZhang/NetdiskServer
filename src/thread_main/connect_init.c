#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

int connect_init_handle(int socket_fd, struct connect_stat_t *connect_stat_arr, int max_connect_num, struct connect_timer_hashnode *connect_timer_arr) {
    int ret = 0;

    // accept 连接
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t addrlen = sizeof(addr);
    int connect_fd = accept(socket_fd, (struct sockaddr *)&addr, &addrlen); // accept 连接
    RET_CHECK_BLACKLIST(-1, connect_fd, "accept");

    // 保存连接状态
    int i = connect_fd % max_connect_num; // 根据描述符获取其应在的连接状态数组成员下标.
    if (connect_stat_arr[i].fd) {
        // 连接数已达到最大, 以至于连接状态数组中无处存放该连接状态.
        close(connect_fd); // 关闭该连接
        log_print("由于连接数已达最大值, 新连接被拒绝.");
        return 0;
    }
    connect_stat_arr[i].fd = connect_fd;
    memcpy(&connect_stat_arr[i].addr, &addr, sizeof(addr));

    // 将新连接放入时间轮定时器
    ret = connect_timer_in(&connect_stat_arr[i], connect_timer_arr);
    RET_CHECK_BLACKLIST(-1, ret, "connect_timer_in");
    printf("new connect fd = %d\n", connect_stat_arr[i].fd);

    return 0;
}
