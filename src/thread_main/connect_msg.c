#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

int connect_msg_handle(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr, struct program_stat_t *program_stat, struct connect_sleep_node *connect_sleep) {
    int ret = 0;
    char buf[1024] = {0};

    // 接收来自客户端的消息类型标志
    ret = connect_msg_fetchtype(connect_stat->fd, buf);
    RET_CHECK_BLACKLIST(-1, ret, "connect_msg_fetchtype");
    if (0 == ret) {
        // 对方已断开连接
        ret = connect_destory(connect_stat, connect_timer_arr, connect_sleep);
        RET_CHECK_BLACKLIST(-1, ret, "connect_destory");
    }

    switch (buf[0]) {
    case MT_REQCONF:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_REQCONF 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_reqconf(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_reqconf 执行出错.");
        }
        break;
    case MT_REGIST:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_REGIST 消息.", connect_stat->fd);
        logging(LOG_INFO, logbuf);
        ret = msg_regist(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_regist 执行出错.");
        }
        break;
    case MT_LOGIN:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_LOGIN 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_login(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_login 执行出错.");
        }
        break;
    case MT_DUPCONN:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_RECONN 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_dupconn(connect_stat, program_stat, connect_sleep);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_dupconn 执行出错.");
        }
        break;
    case MT_COMM_S:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_COMM_S 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        break;
    case MT_COMM_L:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_COMM_L 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        break;
    default:
        break;
    }

    return 0;
}

// 循环接收, 避免因网络数据分包导致的错误
size_t recv_n(int connect_fd, void *buf, size_t len, int flags) {
    int ret = 0;
    char *p = (char *)buf;
    size_t recved_len = 0;
    while (recved_len < len) {
        ret = recv(connect_fd, p + recved_len, len - recved_len, flags);
        RET_CHECK_BLACKLIST(-1, ret, "recv");
        if (0 == ret) {
            return 0; // 当对端断开时, 返回 0.
        }
        recved_len += ret;
    }
    return recved_len; // 正常情况下, 返回接收到的字节数.
}

// 接收来自客户端的消息类型标志
int connect_msg_fetchtype(int connect_fd, void *buf) {
    int ret = 0;
    ret = recv_n(connect_fd, buf, 1, 0);
    return ret;
}