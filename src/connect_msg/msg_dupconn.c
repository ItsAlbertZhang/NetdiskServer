#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_dupconn_recvbuf_t {
    char msgtype;            // 消息类型
    int pretoken;            // token 前缀
    int token_ciprsa_len;    // 下一字段的长度
    char token_ciprsa[1024]; // token 密文
};

struct msg_dupconn_sendbuf_t {
    char msgtype;    // 消息类型
    char approve;    // 批准标志
    int newpretoken; // 可能为新的 token 前缀
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_dupconn_recv(int connect_fd, struct msg_dupconn_recvbuf_t *recvbuf) {
    int ret = 0;

    bzero(recvbuf, sizeof(struct msg_dupconn_recvbuf_t));

    ret = recv_n(connect_fd, &recvbuf->pretoken, sizeof(recvbuf->pretoken), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    ret = recv_n(connect_fd, &recvbuf->token_ciprsa_len, sizeof(recvbuf->token_ciprsa_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->token_ciprsa, recvbuf->token_ciprsa_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_dupconn_send(int connect_fd, struct msg_dupconn_sendbuf_t *sendbuf) {
    int ret = 0;

    // 向客户端发送消息
    sendbuf->msgtype = MT_DUPCONN;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->approve, sizeof(sendbuf->approve), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->newpretoken, sizeof(sendbuf->newpretoken), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

// 尝试从现有连接中拷贝连接
static int connect_cpy(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat, int pretoken, char *token_plain);

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

    // 获取 token
    char token_plain[1024] = {0};
    ret = rsa_decrypt(token_plain, recvbuf.token_ciprsa, program_stat->private_rsa, PRIKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_decrypt");
    sprintf(logbuf, "接收到 fd 为 %d 的连接发来的 token: %s", connect_stat->fd, token_plain);
    logging(LOG_DEBUG, logbuf);

    sendbuf.approve = DISAPPROVE;
    int cpy_complete = 0;
    if (!cpy_complete) {
        // 尝试从现有连接中拷贝连接
        ret = connect_cpy(connect_stat, program_stat, recvbuf.pretoken, token_plain);
        if (0 == ret) {
            sprintf(logbuf, "已成功拷贝旧 fd 为 %d 的连接, 其 token 为 %s, 用户为 %d, 当前工作目录为 %d.", recvbuf.pretoken, connect_stat->token, connect_stat->userid, connect_stat->pwd_id);
            logging(LOG_INFO, logbuf);
            sendbuf.approve = APPROVE;
            cpy_complete = 1;
        }
    }
    if (!cpy_complete) {
        // 尝试从休眠连接中拿出连接信息
        ret = connect_sleep_awake(connect_sleep, token_plain, connect_stat);
        if (0 == ret) {
            sprintf(logbuf, "已成功恢复旧 fd 为 %d 的连接, 其 token 为 %s, 用户为 %d, 当前工作目录为 %d.", recvbuf.pretoken, connect_stat->token, connect_stat->userid, connect_stat->pwd_id);
            logging(LOG_INFO, logbuf);
            sendbuf.approve = APPROVE;
            sendbuf.newpretoken = connect_stat->fd;
            cpy_complete = 1;
        }
    }

    ret = msg_dupconn_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_dupconn_send");

    if (APPROVE == sendbuf.approve) {
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

static int connect_cpy(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat, int pretoken, char *token_plain) {
    int ret = -1;
    // 获取旧连接指针
    int token_connect_diff = (pretoken % program_stat->thread_stat.max_connect_num) - (connect_stat->fd % program_stat->thread_stat.max_connect_num);
    struct connect_stat_t *token_stat = connect_stat + token_connect_diff;
    // 判断两者是否为同一个客户端以及 ip
    if (!strcmp(token_plain, token_stat->token) && !memcmp(&connect_stat->addr.sin_addr, &token_stat->addr.sin_addr, sizeof(struct in_addr))) {
        memcpy(connect_stat->token, token_stat->token, sizeof(connect_stat->token));
        connect_stat->userid = token_stat->userid;
        connect_stat->pwd_id = token_stat->pwd_id;
        ret = 0;
    }
    return ret;
}