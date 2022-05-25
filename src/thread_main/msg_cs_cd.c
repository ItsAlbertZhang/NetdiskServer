#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cs_cd_recvbuf_t {
    char msgtype;   // 消息类型
    int dir_len;    // 下一字段的长度
    char dir[1024]; // 当前目录下的文件
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

    char *dir_p = recvbuf.dir;

    // 如果路径的最后不是以'/'结尾, 则在最后补上'/'
    if ('/' != dir_p[strlen(dir_p) - 1]) {
        dir_p[strlen(dir_p) + 1] = 0;
        dir_p[strlen(dir_p)] = '/';
    }
    // 如果路径是以根目录开始
    if ('/' == *dir_p) {
        connect_stat->pwd_id = 0;
        dir_p++;
    }
    char *dir_p2 = dir_p;
    while (*dir_p) {
        while ('/' != *dir_p2) {
            dir_p2++;
        }
        *dir_p2 = 0; // 将 dir_p2 置 0, msg_cs_cd_mysql_query_id 会认为字符串至此结束

        if (strcmp(dir_p, "..")) {
            ret = msg_cs_cd_mysql_query_id(program_stat->mysql_connect, connect_stat->userid, connect_stat->pwd_id, dir_p);
        } else {
            ret = msg_cs_cd_mysql_query_preid(program_stat->mysql_connect, connect_stat->userid, connect_stat->pwd_id, dir_p);
        }

        if (-1 == ret) {
            break;
        }
        connect_stat->pwd_id = ret;
        dir_p = ++dir_p2;
    }

    if (!*dir_p) {
        sendbuf.approve = APPROVE;
    }

    // 向客户端发送消息
    ret = msg_cs_cd_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_cd_send");

    msg_cs_pwd(connect_stat, program_stat);
    msg_cs_ls(connect_stat, program_stat);

    return 0;
}

static int msg_cs_cd_mysql_query_id(MYSQL *mysql_connect, int userid, int pwd_id, char *dirname) {
    int ret = 0;

    // 查询目录是否存在
    char query_str[1024] = {0};
    sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d AND `filename` = '%s' AND `userid` = %d;", pwd_id, dirname, userid);
    if (1 != libmysql_query_count(mysql_connect, query_str)) {
        return -1;
    }

    // 查询目录的 id
    char id_str[16] = {0};
    char *res_p[] = {&id_str[0]};
    sprintf(query_str, "SELECT `id` FROM `user_file` WHERE `preid` = %d AND `filename` = '%s' AND `userid` = %d;", pwd_id, dirname, userid);
    ret = libmysql_query_1col(mysql_connect, query_str, res_p, 1);
    return atoi(id_str);
}

static int msg_cs_cd_mysql_query_preid(MYSQL *mysql_connect, int userid, int pwd_id, char *dirname) {
    int ret = 0;
    char query_str[1024] = {0};
    char preid_str[16] = {0};
    char *res_p[] = {&preid_str[0]};
    sprintf(query_str, "SELECT `preid` FROM `user_file` WHERE `id` = %d AND `filename` = '%s' AND `userid` = %d;", pwd_id, dirname, userid);
    ret = libmysql_query_1col(mysql_connect, query_str, res_p, 1);
    return atoi(preid_str);
}