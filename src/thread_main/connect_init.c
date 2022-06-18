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
        sprintf(logbuf, "由于连接数已达最大值, 来自 %s 的新连接被拒绝.", inet_ntoa(addr.sin_addr));
        logging(LOG_WARN, logbuf);
        return 0;
    }
    connect_stat_arr[i].fd = connect_fd;
    memcpy(&connect_stat_arr[i].addr, &addr, sizeof(addr));

    // 将新连接放入时间轮定时器
    ret = connect_timer_in(&connect_stat_arr[i], connect_timer_arr);
    RET_CHECK_BLACKLIST(-1, ret, "connect_timer_in");
    sprintf(logbuf, "成功与 %s 建立连接, 连接编号/文件描述符为 %d.", inet_ntoa(connect_stat_arr[i].addr.sin_addr), connect_stat_arr[i].fd);
    logging(LOG_INFO, logbuf);

    // 将新连接添加至 epoll 监听
    ret = epoll_add(connect_stat_arr[i].fd);
    RET_CHECK_BLACKLIST(-1, ret, "epoll_add");

    return 0;
}

int connect_destory(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr, struct queue_t *connect_sleep_queue, int gotosleep) {
    int ret = 0;

    ret = connect_timer_out(connect_stat, connect_timer_arr); // 将连接从时间轮定时器中取出
    RET_CHECK_BLACKLIST(-1, ret, "connect_timer_out");
    ret = epoll_del(connect_stat->fd); // 取消 epoll 监听
    RET_CHECK_BLACKLIST(-1, ret, "epoll_del");
    close(connect_stat->fd); // 关闭连接
    RET_CHECK_BLACKLIST(-1, ret, "close");

    if (gotosleep) {
        connect_sleep_fall(connect_sleep_queue, connect_stat);
    } else {
        sprintf(logbuf, "%d 号连接已主动断开.", connect_stat->fd);
        bzero(connect_stat, sizeof(struct connect_stat_t)); // 清空状态
        logging(LOG_INFO, logbuf);
    }

    return 0;
}