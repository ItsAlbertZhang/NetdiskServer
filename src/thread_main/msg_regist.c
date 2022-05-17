#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_regist_recvbuf_t {
    char msgtype;              // 消息类型
    int username_len;          // 下一字段的长度
    char username[30];         // 用户名
    int pwd_len;               // 下一字段的长度
    char pwd_ciphertext[1024]; // 密码密文
};

struct msg_regist_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_regist_recv(int connect_fd, struct msg_regist_recvbuf_t *recvbuf) {
    int ret = 0;

    bzero(recvbuf, sizeof(struct msg_regist_recvbuf_t));
    ret = recv_n(connect_fd, &recvbuf->username_len, sizeof(recvbuf->username_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->username, recvbuf->username_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->pwd_len, sizeof(recvbuf->pwd_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->pwd_ciphertext, recvbuf->pwd_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_regist_send(int connect_fd, struct msg_regist_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_REGIST;
    ret = send(connect_fd, sendbuf, sizeof(struct msg_regist_sendbuf_t), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

int msg_regist(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_regist_recvbuf_t recvbuf;
    struct msg_regist_sendbuf_t sendbuf;
    bzero(&sendbuf, sizeof(sendbuf));
    bzero(&recvbuf, sizeof(recvbuf));

    // 接收来自客户端的消息
    ret = msg_regist_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_recv");

    // 以下部分未完善, 需要借助数据库中的用户信息表方可实现功能
    char buf[1024] = {0};
    if (recvbuf.pwd_len) {
        rsa_decrypt(buf, recvbuf.pwd_ciphertext, program_stat->private_rsa, PRIKEY);
    }
    for(int i = 0; i < strlen(buf); i++) {
        buf[i] = buf[i] ^ connect_stat->confirm[i];
    }
    char bufp[1024] = {0};
    sprintf(bufp, "%s %s", recvbuf.username, buf);
    logging(LOG_DEBUG, bufp);

    sendbuf.approve = APPROVE; // 在功能实现之前为方便 DEBUG 统一回复同意
    ret = msg_regist_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_send");

    return 0;
}