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
        ret = connect_destory(connect_stat, connect_timer_arr, connect_sleep, 0);
        RET_CHECK_BLACKLIST(-1, ret, "connect_destory");
    }

    switch (buf[0]) {
    case MT_CONNINIT:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CONNINIT 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_conninit(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_conninit 执行出错.");
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
    case MT_CS_PWD:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_PWD 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_pwd(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_pwd 执行出错.");
        }
        break;
    case MT_CS_LS:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_LS 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_ls(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_ls 执行出错.");
        }
        break;
    case MT_CS_CD:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_CD 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_cd(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_cd 执行出错.");
        }
        break;
    case MT_CS_RM:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_RM 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_rm(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_rm 执行出错.");
        }
        break;
    case MT_CS_MV:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_MV 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_mv(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_mv 执行出错.");
        }
        break;
    case MT_CS_CP:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_CP 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_cp(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_cp 执行出错.");
        }
        break;
    case MT_CS_MKDIR:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_MKDIR 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_mkdir(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_mkdir 执行出错.");
        }
        break;
    case MT_CS_RMDIR:
        sprintf(logbuf, "接收到 fd 为 %d 的 MT_CS_RMDIR 消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
        ret = msg_cs_rmdir(connect_stat, program_stat);
        if (-1 == ret) {
            logging(LOG_ERROR, "msg_cs_rmdir 执行出错.");
        }
        break;
    default:
        sprintf(logbuf, "接收到 fd 为 %d 的未知消息.", connect_stat->fd);
        logging(LOG_DEBUG, logbuf);
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