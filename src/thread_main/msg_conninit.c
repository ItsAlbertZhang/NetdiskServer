#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_conninit_recvbuf_t {
    char msgtype;             // 消息类型
    int clientpubrsa_len;     // 下一字段的长度
    char clientpubrsa[1024];  // 客户端公钥字符串
    int serverpub_md5_len;    // 下一字段的长度
    char serverpub_md5[1024]; // 客户端本地存储的服务端公钥字符串的 MD5 校验码
};

struct msg_conninit_sendbuf_t {
    char msgtype;            // 消息类型
    int pretoken;            // token 前缀, 其实质为最近一次连接时服务端的文件描述符
    int token_ciprsa_len;    // 下一字段的长度
    char token_ciprsa[1024]; // token 密文
    int serverpubrsa_len;    // 下一字段的长度
    char serverpubrsa[1024]; // 服务端公钥. 当无需传输时, 此字段与上一字段置空.
};

static int msg_conninit_recv(int connect_fd, struct msg_conninit_recvbuf_t *recvbuf) {
    int ret = 0;

    bzero(recvbuf, sizeof(struct msg_conninit_recvbuf_t));
    ret = recv_n(connect_fd, &recvbuf->clientpubrsa_len, sizeof(recvbuf->clientpubrsa_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->clientpubrsa, recvbuf->clientpubrsa_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->serverpub_md5_len, sizeof(recvbuf->serverpub_md5_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->serverpub_md5, recvbuf->serverpub_md5_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_conninit_send(int connect_fd, struct msg_conninit_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CONNINIT;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->pretoken, sizeof(sendbuf->pretoken), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->token_ciprsa_len, sizeof(sendbuf->token_ciprsa_len) + sendbuf->token_ciprsa_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->serverpubrsa_len, sizeof(sendbuf->serverpubrsa_len) + sendbuf->serverpubrsa_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

int msg_conninit(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_conninit_recvbuf_t recvbuf;
    struct msg_conninit_sendbuf_t sendbuf;
    bzero(&sendbuf, sizeof(sendbuf));
    bzero(&recvbuf, sizeof(recvbuf));
    sendbuf.msgtype = MT_CONNINIT;

    // 接收来自客户端的消息
    ret = msg_conninit_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_conninit_recv");

    // 处理传来的客户端公钥
    RSA *clientrsa = NULL;
    ret = rsa_str2rsa(recvbuf.clientpubrsa, &clientrsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_str2rsa");

    // 生成 token
    sendbuf.pretoken = connect_stat->fd;
    random_gen_str(connect_stat->token, 30, connect_stat->fd);
    sprintf(logbuf, "已为 fd 为 %d 的连接生成 token: %s", connect_stat->fd, connect_stat->token);
    logging(LOG_DEBUG, logbuf);
    // 将 token 加密
    sendbuf.token_ciprsa_len = rsa_encrypt(connect_stat->token, sendbuf.token_ciprsa, clientrsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, sendbuf.token_ciprsa_len, "rsa_encrypt");

    // 处理传来的服务端公钥 MD5 校验码
    ret = rsa_rsa2str(sendbuf.serverpubrsa, program_stat->public_rsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_rsa2str");
    // 对比计算得出的服务端公钥 MD5 校验码与客户端发来的信息中的校验码
    if (strcmp(MD5(sendbuf.serverpubrsa, strlen(sendbuf.serverpubrsa), NULL), recvbuf.serverpub_md5)) {
        // strcmp 的结果不为 0, 说明两者不同, 此时需要向客户端发送公钥.
        sendbuf.serverpubrsa_len = strlen(sendbuf.serverpubrsa);
    }

    // 向客户端发送消息
    ret = msg_conninit_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_send");

    return 0;
}