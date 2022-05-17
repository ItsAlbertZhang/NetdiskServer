#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_reqconf_recvbuf_t {
    char msgtype;             // 消息类型
    int serverpub_md5_len;    // 下一字段的长度
    char serverpub_md5[1024]; // 客户端本地存储的服务端公钥字符串的 MD5 校验码
};

struct msg_reqconf_sendbuf_t {
    char msgtype;             // 消息类型
    int confirm_len;          // 下一字段的长度
    char confirm[64];         // 会话确认码
    int serverpub_str_len;    // 下一字段的长度
    char serverpub_str[1024]; // 服务端公钥. 当无需传输时, 此字段与上一字段置空.
};

static int msg_reqconf_recv(int connect_fd, struct msg_reqconf_recvbuf_t *recvbuf) {
    int ret = 0;

    bzero(recvbuf, sizeof(struct msg_reqconf_recvbuf_t));
    ret = recv_n(connect_fd, &recvbuf->serverpub_md5_len, sizeof(recvbuf->serverpub_md5_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->serverpub_md5, recvbuf->serverpub_md5_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_reqconf_send(int connect_fd, struct msg_reqconf_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_REQCONF;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->confirm_len, sizeof(sendbuf->confirm_len) + sendbuf->confirm_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->serverpub_str_len, sizeof(sendbuf->serverpub_str_len) + sendbuf->serverpub_str_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

int msg_reqconf(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_reqconf_recvbuf_t recvbuf;
    struct msg_reqconf_sendbuf_t sendbuf;
    bzero(&sendbuf, sizeof(sendbuf));
    bzero(&recvbuf, sizeof(recvbuf));
    sendbuf.msgtype = MT_REQCONF;

    // 接收来自客户端的消息
    ret = msg_reqconf_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_reqconf_recv");

    // 生成确认码
    sendbuf.confirm_len = 30;
    random_gen_str(sendbuf.confirm, sendbuf.confirm_len, connect_stat->fd);
    strcpy(connect_stat->confirm, sendbuf.confirm); // 将确认码复制到连接信息中
    logging(LOG_DEBUG, connect_stat->confirm);

    // 处理公钥
    ret = rsa_rsa2str(sendbuf.serverpub_str, program_stat->public_rsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_rsa2str");
    // 对比计算得出的服务端公钥 MD5 校验码与客户端发来的信息中的校验码
    if (strcmp(MD5(sendbuf.serverpub_str, strlen(sendbuf.serverpub_str), NULL), recvbuf.serverpub_md5)) {
        // strcmp 的结果不为 0, 说明两者不同, 此时需要向客户端发送公钥.
        sendbuf.serverpub_str_len = strlen(sendbuf.serverpub_str);
    }

    // 向客户端发送消息
    ret = msg_reqconf_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_send");

    return 0;
}