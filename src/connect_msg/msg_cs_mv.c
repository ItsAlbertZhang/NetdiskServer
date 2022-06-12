#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cs_mv_recvbuf_t {
    char msgtype;        // 消息类型
    int mvsource_len;    // 下一字段的长度
    char mvsource[1024]; // 原文件或目录
    int mvdir_len;       // 下一字段的长度
    char mvdir[1024];    // 目标路径
    int rename_len;      // 下一字段的长度
    char rename[64];     // 重命名
};

struct msg_cs_mv_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_cs_mv_recv(int connect_fd, struct msg_cs_mv_recvbuf_t *recvbuf) {
    int ret = 0;

    ret = recv_n(connect_fd, &recvbuf->mvsource_len, sizeof(recvbuf->mvsource_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->mvsource, recvbuf->mvsource_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->mvdir_len, sizeof(recvbuf->mvdir_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->mvdir, recvbuf->mvdir_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->rename_len, sizeof(recvbuf->rename_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->rename, recvbuf->rename_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_cs_mv_send(int connect_fd, struct msg_cs_mv_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CS_MV;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->approve, sizeof(sendbuf->approve), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

int msg_cs_mv(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_cs_mv_recvbuf_t recvbuf;
    struct msg_cs_mv_sendbuf_t sendbuf;
    bzero(&recvbuf, sizeof(recvbuf));
    bzero(&sendbuf, sizeof(sendbuf));

    // 接收来自客户端的消息
    ret = msg_cs_mv_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_mv_recv");

    int id_source = connect_stat->pwd_id, id_dir = connect_stat->pwd_id;
    int check_flag = 1;
    // 验证原文件或目录的合法性
    if (check_flag) {
        ret = msg_lib_path2id(recvbuf.mvsource, &id_source, program_stat->mysql_connect);
        check_flag = -1 != ret;
    }
    // 验证目标路径的合法性
    if (check_flag) {
        ret = msg_lib_path2id(recvbuf.mvdir, &id_dir, program_stat->mysql_connect);
        check_flag = TYPE_DIR == ret;
    }

    if (check_flag) {
        char query_str[1024] = {0};
        if (recvbuf.rename_len) {
            sprintf(query_str, "UPDATE `user_file` SET `preid` = %d, `filename` = '%s' WHERE `id` = %d;", id_dir, recvbuf.rename, id_source);
        } else {
            sprintf(query_str, "UPDATE `user_file` SET `preid` = %d WHERE `id` = %d;", id_dir, id_source);
        }
        ret = mysql_query(program_stat->mysql_connect, query_str);
        RET_CHECK_BLACKLIST(-1, ret, "mysql_query");
        sendbuf.approve = APPROVE;
    }

    // 向客户端发送消息
    ret = msg_cs_mv_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_mv_send");

    return 0;
}