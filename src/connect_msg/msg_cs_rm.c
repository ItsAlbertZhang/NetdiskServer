#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cs_rm_recvbuf_t {
    char msgtype;   // 消息类型
    char rmtype;    // 删除类型
    int dir_len;    // 下一字段的长度
    char dir[1024]; // 文件或目录名
};

struct msg_cs_rm_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

#define RM_NULL 0
#define RM_R 1

static int msg_cs_rm_recv(int connect_fd, struct msg_cs_rm_recvbuf_t *recvbuf) {
    int ret = 0;

    ret = recv_n(connect_fd, &recvbuf->rmtype, sizeof(recvbuf->rmtype), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->dir_len, sizeof(recvbuf->dir_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->dir, recvbuf->dir_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_cs_rm_send(int connect_fd, struct msg_cs_rm_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CS_RM;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->approve, sizeof(sendbuf->approve), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

static int msg_cs_rm_r(MYSQL *mysql_connect, int rmid, int userid);

int msg_cs_rm(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_cs_rm_recvbuf_t recvbuf;
    struct msg_cs_rm_sendbuf_t sendbuf;
    bzero(&recvbuf, sizeof(recvbuf));
    bzero(&sendbuf, sizeof(sendbuf));

    // 接收来自客户端的消息
    ret = msg_cs_rm_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_rm_recv");

    int rmid = connect_stat->pwd_id;
    ret = msg_lib_path2id(recvbuf.dir, &rmid, program_stat->mysql_connect);
    if (TYPE_FILE == ret && recvbuf.rmtype == RM_NULL) {
        // 普通的 rm 删除, 则 rmid 必须为文件(而非目录)
        char query_str[1024] = {0};
        if (TYPE_FILE == ret) {
            sprintf(query_str, "DELETE FROM `user_file` WHERE `id` = %d AND `userid` = %d;", rmid, connect_stat->userid);
            ret = mysql_query(program_stat->mysql_connect, query_str);
            RET_CHECK_BLACKLIST(-1, ret, "mysql_query");
            sendbuf.approve = APPROVE;
        }
    }
    if (-1 != ret && recvbuf.rmtype == RM_R) {
        // 递归的 rm 删除. rmid 可以为文件, 也可以为目录.
        ret = msg_cs_rm_r(program_stat->mysql_connect, rmid, connect_stat->userid);
        RET_CHECK_BLACKLIST(-1, ret, "msg_cs_rm_r");
        sendbuf.approve = APPROVE;
    }

    // 向客户端发送消息
    ret = msg_cs_rm_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_rm_send");

    return 0;
}

static int msg_cs_rm_r(MYSQL *mysql_connect, int rmid, int userid) {
    int ret = 0;

    char query_str[1024] = {0};
    char type_str[4] = {0};
    char *res_p[] = {&type_str[0]};
    sprintf(query_str, "SELECT `type` FROM `user_file` WHERE `id` = %d AND `userid` = %d;", rmid, userid);
    ret = libmysql_query_1col(mysql_connect, query_str, res_p, 1);
    if (0 == strcmp(type_str, "f")) {
        sprintf(query_str, "DELETE FROM `user_file` WHERE `id` = %d AND `userid` = %d;", rmid, userid);
        ret = mysql_query(mysql_connect, query_str);
        RET_CHECK_BLACKLIST(-1, ret, "mysql_query");
    }
    ret = 0;
    if (0 == strcmp(type_str, "d")) {
        // 获取当前目录下的文件数量
        char query_str[1024] = {0};
        sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d AND `userid` = %d;", rmid, userid);
        int filenum = libmysql_query_11count(mysql_connect, query_str);
        if (filenum) {
            // 申请空间
            char **fileid = (char **)malloc(sizeof(char *) * filenum);
            fileid[0] = (char *)malloc(16 * filenum);
            for (int i = 0; i < filenum; i++) {
                fileid[i] = fileid[0] + i * 16;
            }
            // 在数据库中查询
            sprintf(query_str, "SELECT `id` FROM `user_file` WHERE `preid` = %d AND `userid` = %d;", rmid, userid);
            ret = libmysql_query_1col(mysql_connect, query_str, fileid, filenum);
            RET_CHECK_BLACKLIST(ret, -1, "libmysql_query_1col");
            // 递归进行删除
            for (int i = 0; i < filenum; i++) {
                ret = msg_cs_rm_r(mysql_connect, atoi(fileid[i]), userid);
                RET_CHECK_BLACKLIST(-1, ret, "msg_cs_rm_r");
            }
            // 释放空间
            free(fileid[0]);
            free(fileid);
        }
        // 再次获取当前目录下的文件数量
        sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d AND `userid` = %d;", rmid, userid);
        filenum = libmysql_query_11count(mysql_connect, query_str);
        if (0 == filenum) {
            // 删除当前目录
            sprintf(query_str, "DELETE FROM `user_file` WHERE `id` = %d AND `userid` = %d;", rmid, userid);
            ret = mysql_query(mysql_connect, query_str);
            RET_CHECK_BLACKLIST(-1, ret, "mysql_query");
            ret = 0;
        } else {
            ret = -1;
        }
    }

    return ret;
}