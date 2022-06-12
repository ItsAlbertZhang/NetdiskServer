#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cs_rmdir_recvbuf_t {
    char msgtype;     // 消息类型
    int dirname_len;  // 下一字段的长度
    char dirname[64]; // 原文件或目录
};

struct msg_cs_rmdir_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_cs_rmdir_recv(int connect_fd, struct msg_cs_rmdir_recvbuf_t *recvbuf) {
    int ret = 0;

    ret = recv_n(connect_fd, &recvbuf->dirname_len, sizeof(recvbuf->dirname_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->dirname, recvbuf->dirname_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_cs_rmdir_send(int connect_fd, struct msg_cs_rmdir_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CS_RMDIR;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->approve, sizeof(sendbuf->approve), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

int msg_cs_rmdir(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_cs_rmdir_recvbuf_t recvbuf;
    struct msg_cs_rmdir_sendbuf_t sendbuf;
    bzero(&recvbuf, sizeof(recvbuf));
    bzero(&sendbuf, sizeof(sendbuf));

    // 接收来自客户端的消息
    ret = msg_cs_rmdir_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_rmdir_recv");

    char query_str[1024] = {0};
    // 查询该目录名对应的文件 id
    sprintf(query_str, "SELECT `id` FROM `user_file` WHERE `preid` = %d AND `type` = 'd' AND `filename` = '%s';", connect_stat->pwd_id, recvbuf.dirname);
    int dirid = libmysql_query_11count(program_stat->mysql_connect, query_str);
    if (dirid > 0) {
        // 查询该目录下是否还有文件
        sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d AND `userid` = %d;", dirid, connect_stat->userid);
        ret = libmysql_query_11count(program_stat->mysql_connect, query_str);
        if (0 == ret) {
            sprintf(query_str, "DELETE FROM `user_file` WHERE `id` = %d AND `userid` = %d;", dirid, connect_stat->userid);
            ret = mysql_query(program_stat->mysql_connect, query_str);
            RET_CHECK_BLACKLIST(-1, ret, "mysql_query");
            sendbuf.approve = APPROVE;
        }
    }

    // 向客户端发送消息
    ret = msg_cs_rmdir_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_rmdir_send");

    return 0;
}