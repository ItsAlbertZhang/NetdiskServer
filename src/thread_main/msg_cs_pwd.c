#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_regist_recvbuf_t {
    char msgtype; // 消息类型
};

struct msg_regist_sendbuf_t {
    char msgtype;   // 消息类型
    int pwd_len;    // 下一字段的长度
    char pwd[1024]; // 当前工作目录
};

static int msg_cs_pwd_recv(int connect_fd, struct msg_regist_recvbuf_t *recvbuf) {
    int ret = 0;
    return 0;
}

static int msg_cs_pwd_send(int connect_fd, struct msg_regist_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CS_PWD;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->pwd_len, sizeof(sendbuf->pwd_len), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, sendbuf->pwd, sendbuf->pwd_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

static int msg_cs_pwd_getpwd(char *pwd, MYSQL *mysql_connect, int userid, int pwd_id);

int msg_cs_pwd(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_regist_recvbuf_t recvbuf;
    struct msg_regist_sendbuf_t sendbuf;
    bzero(&sendbuf, sizeof(sendbuf));
    bzero(&recvbuf, sizeof(recvbuf));

    // 无需接收来自客户端的消息

    // 查询数据库: 获取当前工作目录
    int pwd_id = connect_stat->pwd_id;
    ret = msg_cs_pwd_getpwd(sendbuf.pwd, program_stat->mysql_connect, connect_stat->userid, connect_stat->pwd_id);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_pwd_getpwd");
    sendbuf.pwd_len = strlen(sendbuf.pwd);

    // 向客户端发送信息
    ret = msg_cs_pwd_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_pwd_send");
}

static int msg_cs_pwd_mysql_query(MYSQL *mysql_connect, int userid, int pwd_id, char *filename) {
    int ret = 0, preid = -1;

    char preid_str[10] = {0};
    char *res_p[] = {&preid_str[0], &filename[0]};
    char query_str[1024] = {0};
    sprintf(query_str, "SELECT `preid`, `filename` FROM `user_file` WHERE `id` = %d AND `userid` = %d;", pwd_id, userid);
    ret = libmysql_query_1row(mysql_connect, query_str, res_p, 2);
    if (1 != ret) {
        RET_CHECK_BLACKLIST(0, 0, "libmysql_query_1col");
    } else {
        preid = atoi(preid_str);
    }

    return preid;
}

static int msg_cs_pwd_getpwd(char *pwd, MYSQL *mysql_connect, int userid, int pwd_id) {
    if (0 == pwd_id) {
        strcpy(pwd, "~/");
    } else {
        char pwdbuf[64] = {0};
        int pre_pwd_id = msg_cs_pwd_mysql_query(mysql_connect, userid, pwd_id, pwdbuf);
        RET_CHECK_BLACKLIST(-1, pre_pwd_id, "msg_cs_pwd_mysql_query");
        msg_cs_pwd_getpwd(pwd, mysql_connect, userid, pre_pwd_id);
        strcpy(pwd, pwdbuf);
        strcpy(pwd, "/");
    }
    return 0;
}