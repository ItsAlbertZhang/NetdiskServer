#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cs_ls_recvbuf_t {
    char msgtype; // 消息类型
};

struct msg_cs_ls_sendbuf_t {
    char msgtype;   // 消息类型
    int res_len;    // 下一字段的长度
    char res[4096]; // 当前目录下的文件
};

static int msg_cs_ls_recv(int connect_fd, struct msg_cs_ls_recvbuf_t *recvbuf) {
    int ret = 0;
    return 0;
}

static int msg_cs_ls_send(int connect_fd, struct msg_cs_ls_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CS_LS;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->res_len, sizeof(sendbuf->res_len), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, sendbuf->res, sendbuf->res_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

static int msg_cs_ls_mysql_query(MYSQL *mysql_connect, int userid, int pwd_id, char *res);

int msg_cs_ls(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_cs_ls_recvbuf_t recvbuf;
    struct msg_cs_ls_sendbuf_t sendbuf;
    bzero(&recvbuf, sizeof(recvbuf));
    bzero(&sendbuf, sizeof(sendbuf));

    // 无需接受来自客户端的消息

    // 查询数据库: 获取当前目录下的文件
    ret = msg_cs_ls_mysql_query(program_stat->mysql_connect, connect_stat->userid, connect_stat->pwd_id, sendbuf.res);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_ls_mysql_query");
    sendbuf.res_len = strlen(sendbuf.res);

    // 向客户端发送信息
    ret = msg_cs_ls_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_ls_send");

    return 0;
}

#define FILENAMEMAXSIZE 64
#define FILETYPEMAXSIZE 4

static int msg_cs_ls_mysql_query(MYSQL *mysql_connect, int userid, int pwd_id, char *res) {
    int ret = 0;
    // 获取当前目录下的文件数量
    char query_str[1024] = {0};
    sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d AND `userid` = %d;", pwd_id, userid);
    int filenum = libmysql_query_11count(mysql_connect, query_str);
    // 申请空间
    char **filename = (char **)malloc(sizeof(char *) * filenum);
    filename[0] = (char *)malloc(FILENAMEMAXSIZE * filenum);
    for (int i = 0; i < filenum; i++) {
        filename[i] = filename[0] + i * FILENAMEMAXSIZE;
    }
    char **filetype = (char **)malloc(sizeof(char *) * filenum);
    filetype[0] = (char *)malloc(FILETYPEMAXSIZE * filenum);
    for (int i = 0; i < filenum; i++) {
        filetype[i] = filetype[0] + i * FILETYPEMAXSIZE;
    }
    // 在数据库中查询
    sprintf(query_str, "SELECT `filename` FROM `user_file` WHERE `preid` = %d AND `userid` = %d;", pwd_id, userid);
    ret = libmysql_query_1col(mysql_connect, query_str, filename, filenum);
    RET_CHECK_BLACKLIST(ret, -1, "libmysql_query_1col");
    sprintf(query_str, "SELECT `type` FROM `user_file` WHERE `preid` = %d AND `userid` = %d;", pwd_id, userid);
    ret = libmysql_query_1col(mysql_connect, query_str, filetype, filenum);
    RET_CHECK_BLACKLIST(ret, -1, "libmysql_query_1col");
    // 拼接结果
    for (int i = 0; i < filenum; i++) {
        if (!strcmp(filetype[i], "d")) {
            strcat(res, "\e[34m\e[1m");
        }
        strcat(res, filename[i]);
        strcat(res, "\e[0m\n");
    }
    res[strlen(res) - 1] = 0;
    // 释放空间
    free(filename[0]);
    free(filename);
    free(filetype[0]);
    free(filetype);
    return 0;
}