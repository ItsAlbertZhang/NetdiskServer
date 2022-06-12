#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cs_cd_recvbuf_t {
    char msgtype;   // 消息类型
    int dir_len;    // 下一字段的长度
    char dir[1024]; // 目录名
};

struct msg_cs_cd_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_cs_cd_recv(int connect_fd, struct msg_cs_cd_recvbuf_t *recvbuf) {
    int ret = 0;

    ret = recv_n(connect_fd, &recvbuf->dir_len, sizeof(recvbuf->dir_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->dir, recvbuf->dir_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_cs_cd_send(int connect_fd, struct msg_cs_cd_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CS_CD;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->approve, sizeof(sendbuf->approve), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

static int msg_cs_cd_mysql_query_id(MYSQL *mysql_connect, int userid, int pwd_id, char *dirname);

static int msg_cs_cd_mysql_query_preid(MYSQL *mysql_connect, int userid, int pwd_id, char *dirname);

int msg_cs_cd(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_cs_cd_recvbuf_t recvbuf;
    struct msg_cs_cd_sendbuf_t sendbuf;
    bzero(&recvbuf, sizeof(recvbuf));
    bzero(&sendbuf, sizeof(sendbuf));

    // 接收来自客户端的消息
    ret = msg_cs_cd_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_cd_recv");

    ret = msg_lib_path2id(recvbuf.dir, &connect_stat->pwd_id, program_stat->mysql_connect);
    if (-1 != ret) {
        sendbuf.approve = APPROVE;
    }

    // 向客户端发送消息
    ret = msg_cs_cd_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_cd_send");

    msg_cs_pwd(connect_stat, program_stat);
    msg_cs_ls(connect_stat, program_stat);

    return 0;
}