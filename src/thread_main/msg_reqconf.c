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
    char msgtype;                // 消息类型
    int confirm_len;             // 下一字段的长度
    char confirm[64];            // 会话确认码
    int token_len;               // 下一字段的长度
    char token_ciphertext[1024]; // token 密文, 用于客户端建立新连接时的短验证
    int serverpub_str_len;       // 下一字段的长度
    char serverpub_str[1024];    // 服务端公钥. 当无需传输时, 此字段与上一字段置空.
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
    ret = send(connect_fd, &sendbuf->confirm_len, sizeof(sendbuf->confirm_len) + sendbuf->confirm_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->token_len, sizeof(sendbuf->token_len) + sendbuf->token_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->serverpub_str_len, sizeof(sendbuf->serverpub_str_len) + sendbuf->serverpub_str_len, MSG_NOSIGNAL);
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

    // 生成确认码
    sendbuf.confirm_len = 30;
    random_gen_str(sendbuf.confirm, sendbuf.confirm_len, connect_stat->fd);
    strcpy(connect_stat->confirm, sendbuf.confirm); // 将确认码存入连接状态中
    sprintf(logbuf, "已为 fd 为 %d 的连接生成确认码: %s", connect_stat->fd, connect_stat->confirm);
    logging(LOG_DEBUG, logbuf);

    // 处理传来的客户端公钥
    RSA *clientrsa = NULL;
    ret = rsa_str2rsa(recvbuf.clientpubrsa, &clientrsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_str2rsa");

    // 生成 token 并加密
    char token[64] = {0};
    connect_stat->init_time = time(NULL); // 将初次连接时间存入连接状态中
    sprintf(token, "%d %ld", connect_stat->fd, connect_stat->init_time);
    sprintf(logbuf, "已为 fd 为 %d 的连接生成 token: %s", connect_stat->fd, token);
    logging(LOG_DEBUG, logbuf);
    sendbuf.token_len = rsa_encrypt(token, sendbuf.token_ciphertext, clientrsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, sendbuf.token_len, "rsa_encrypt");

    // 处理传来的服务端公钥 MD5 校验码
    ret = rsa_rsa2str(sendbuf.serverpub_str, program_stat->public_rsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_rsa2str");
    // 对比计算得出的服务端公钥 MD5 校验码与客户端发来的信息中的校验码
    if (strcmp(MD5(sendbuf.serverpub_str, strlen(sendbuf.serverpub_str), NULL), recvbuf.serverpub_md5)) {
        // strcmp 的结果不为 0, 说明两者不同, 此时需要向客户端发送公钥.
        sendbuf.serverpub_str_len = strlen(sendbuf.serverpub_str);
    }

    // 向客户端发送消息
    ret = msg_conninit_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_send");

    return 0;
}