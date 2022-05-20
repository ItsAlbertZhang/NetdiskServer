#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_dupconn_recvbuf_t {
    char msgtype;                // 消息类型
    int token_len;               // 下一字段的长度
    char token_ciphertext[1024]; // 密码密文
};

struct msg_dupconn_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_dupconn_recv(int connect_fd, struct msg_dupconn_recvbuf_t *recvbuf) {
    int ret = 0;

    bzero(recvbuf, sizeof(struct msg_dupconn_recvbuf_t));
    ret = recv_n(connect_fd, &recvbuf->token_len, sizeof(recvbuf->token_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->token_ciphertext, recvbuf->token_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_dupconn_send(int connect_fd, struct msg_dupconn_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_DUPCONN;
    ret = send(connect_fd, sendbuf, sizeof(struct msg_dupconn_sendbuf_t), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

// 尝试从现有连接中拷贝连接
static int connect_cpy(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat, int tokenfd, time_t tokentime);

int msg_dupconn(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat, struct connect_sleep_node *connect_sleep) {
    int ret = 0;

    // 准备资源
    struct msg_dupconn_recvbuf_t recvbuf;
    struct msg_dupconn_sendbuf_t sendbuf;
    bzero(&sendbuf, sizeof(sendbuf));
    bzero(&recvbuf, sizeof(recvbuf));

    // 接收来自客户端的消息
    ret = msg_dupconn_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_dupconn_recv");
    printf("here1\n");

    // 解密
    char token[1024] = {0};
    ret = rsa_decrypt(token, recvbuf.token_ciphertext, program_stat->private_rsa, PRIKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_decrypt");
    int tokenfd = 0;
    time_t tokentime = 0;
    sscanf(token, "%d%ld", &tokenfd, &tokentime);
    sprintf(logbuf, "接收到 fd 为 %d 的连接发来的 token: fd = %d, time = %ld", connect_stat->fd, tokenfd, tokentime);
    logging(LOG_DEBUG, logbuf);

    sendbuf.approve = DISAPPROVE;
    // 尝试从现有连接中拷贝连接
    ret = connect_cpy(connect_stat, program_stat, tokenfd, tokentime);
    if (0 == ret) {
        sprintf(logbuf, "已成功拷贝 fd 为 %d 的连接, 其确认码为 %s, 用户为 %d, 当前工作目录为 %d.", connect_stat->fd, connect_stat->confirm, connect_stat->userid, connect_stat->pwd_id);
        logging(LOG_INFO, logbuf);
        sendbuf.approve = APPROVE;
    }
    // 尝试从休眠连接中拿出连接信息
    ret = connect_sleep_awake(connect_sleep, tokentime, connect_stat);
    if (0 == ret) {
        sprintf(logbuf, "已成功恢复初次连接时间为 %ld 的连接, 其确认码为 %s, 用户为 %d, 当前工作目录为 %d.", connect_stat->init_time, connect_stat->confirm, connect_stat->userid, connect_stat->pwd_id);
        logging(LOG_INFO, logbuf);
        sendbuf.approve = APPROVE;
    }

    // 向客户端发送消息
    ret = msg_dupconn_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_dupconn_send");

    if (APPROVE == sendbuf.approve) {
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

static int connect_cpy(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat, int tokenfd, time_t tokentime) {
    int ret = -1;
    if (tokenfd != connect_stat->fd) {
        // 获取旧连接指针
        int max_connect_num = program_stat->thread_stat.pth_num + program_stat->thread_stat.thread_resource.queue->len;
        int token_connect_diff = (tokenfd % max_connect_num) - (connect_stat->fd % max_connect_num);
        struct connect_stat_t *token_stat = connect_stat + token_connect_diff;
        if (tokentime == token_stat->init_time && !memcmp(&connect_stat->addr.sin_addr, &token_stat->addr.sin_addr, sizeof(connect_stat->addr.sin_addr))) {
            // 两者为同一客户端
            memcpy(connect_stat->confirm, token_stat->confirm, sizeof(connect_stat->confirm) + sizeof(connect_stat->init_time) + sizeof(connect_stat->userid) + sizeof(connect_stat->pwd_id));
            ret = 0;
        }
    }
    return ret;
}