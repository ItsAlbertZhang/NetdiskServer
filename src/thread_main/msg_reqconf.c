#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

int msg_reqconf(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 接收
    int recvbuf_msglen = 0;
    char recvbuf_msg[1024] = {0};

    ret = recv_n(connect_stat->fd, &recvbuf_msglen, sizeof(recvbuf_msglen), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv_n");
    ret = recv_n(connect_stat->fd, &recvbuf_msg, recvbuf_msglen, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv_n");

    // 处理确认码
    char sendbuf_confirm[30] = {0};
    int sendbuf_confirmlen = sizeof(sendbuf_confirm);
    random_gen_str(sendbuf_confirm, sendbuf_confirmlen, connect_stat->fd);

    // 处理公钥
    char sendbuf_rsastr[4096] = {0};
    ret = rsa_rsa2str(sendbuf_rsastr, program_stat->public_rsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_rsa2str");
    int sendbuf_rsastrlen = strlen(sendbuf_rsastr);

    if (recvbuf_msglen && !strcmp(MD5(sendbuf_rsastr, sendbuf_rsastrlen, NULL), recvbuf_msg)) {
        sendbuf_rsastrlen = 0;
    }

    // 发送
    char sendbuf_msgtype = MT_REQCONF;
    ret = send(connect_stat->fd, &sendbuf_msgtype, sizeof(sendbuf_msgtype), 0);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_stat->fd, &sendbuf_confirmlen, sizeof(sendbuf_confirmlen), 0);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_stat->fd, sendbuf_confirm, sendbuf_confirmlen, 0);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_stat->fd, &sendbuf_rsastrlen, sizeof(sendbuf_rsastrlen), 0);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    if (sendbuf_rsastrlen) {
        ret = send(connect_stat->fd, sendbuf_rsastr, sendbuf_rsastrlen, 0);
        RET_CHECK_BLACKLIST(-1, ret, "send");
    }

    return 0;
}