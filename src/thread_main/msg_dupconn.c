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

    // 解密
    char token[1024] = {0};
    ret = rsa_decrypt(token, recvbuf.token_ciphertext, program_stat->private_rsa, PRIKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_decrypt");
    int oldfd = 0;
    time_t oldtime = 0;
    sscanf(token, "%d%ld", &oldfd, &oldtime);

    // 从休眠连接中拿出连接信息
    ret = connect_sleep_awake(connect_sleep, oldtime, connect_stat);
    RET_CHECK_BLACKLIST(-1, ret, "connect_sleep_awake");
    sprintf(logbuf, "已成功恢复初次连接时间为 %ld 的连接, 其确认码为 %s, 用户为 %d, 当前工作目录为 %d.", connect_stat->init_time, connect_stat->confirm, connect_stat->user_id, connect_stat->pwd_id);
    logging(LOG_DEBUG, logbuf);

    // 向客户端发送消息
    sendbuf.approve = APPROVE;
    ret = msg_dupconn_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_dupconn_send");

    return 0;
}