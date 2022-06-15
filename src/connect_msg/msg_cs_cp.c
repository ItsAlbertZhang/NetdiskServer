#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cs_cp_recvbuf_t {
    char msgtype;        // 消息类型
    int cpsource_len;    // 下一字段的长度
    char cpsource[1024]; // 原文件或目录
    int cpdir_len;       // 下一字段的长度
    char cpdir[1024];    // 目标路径
    int rename_len;      // 下一字段的长度
    char rename[64];     // 重命名
};

struct msg_cs_cp_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_cs_cp_recv(int connect_fd, struct msg_cs_cp_recvbuf_t *recvbuf) {
    int ret = 0;

    ret = recv_n(connect_fd, &recvbuf->cpsource_len, sizeof(recvbuf->cpsource_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->cpsource, recvbuf->cpsource_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->cpdir_len, sizeof(recvbuf->cpdir_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->cpdir, recvbuf->cpdir_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->rename_len, sizeof(recvbuf->rename_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->rename, recvbuf->rename_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_cs_cp_send(int connect_fd, struct msg_cs_cp_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CS_CP;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->approve, sizeof(sendbuf->approve), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

// 递归复制
static int msg_cs_cp_r(MYSQL *mysql_connect, int id_source, int id_dir, const char *rename);

int msg_cs_cp(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_cs_cp_recvbuf_t recvbuf;
    struct msg_cs_cp_sendbuf_t sendbuf;
    bzero(&recvbuf, sizeof(recvbuf));
    bzero(&sendbuf, sizeof(sendbuf));

    // 接收来自客户端的消息
    ret = msg_cs_cp_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_cp_recv");

    char query_str[1024] = {0};
    int id_source = connect_stat->pwd_id, id_dir = connect_stat->pwd_id;
    int check_flag = 1;
    // 验证源文件或目录的合法性
    if (check_flag) {
        ret = msg_lib_path2id(recvbuf.cpsource, &id_source, program_stat->mysql_connect);
        check_flag = -1 != ret;
    }
    // 验证目标路径的合法性
    if (check_flag) {
        ret = msg_lib_path2id(recvbuf.cpdir, &id_dir, program_stat->mysql_connect);
        check_flag = TYPE_DIR == ret;
    }
    // 目标路径不能在源文件的子目录下, 否则可能产生无限递归
    if (check_flag) {
        int id_temp = id_dir;
        while (id_temp && id_temp != id_source) {
            sprintf(query_str, "SELECT `preid` FROM `user_file` WHERE `id` = %d;", id_temp);
            id_temp = libmysql_query_11count(program_stat->mysql_connect, query_str);
        }
        check_flag = 0 == id_temp;
    }
    // 验证文件名是否重复
    char filename[64] = {0};
    if (check_flag) {
        // 获取源文件文件名
        if (recvbuf.rename_len) {
            strcpy(filename, recvbuf.rename);
        } else {
            char *res_p[] = {&filename[0]};
            sprintf(query_str, "SELECT `filename` FROM `user_file` WHERE `id` = %d;", id_source);
            ret = libmysql_query_1col(program_stat->mysql_connect, query_str, res_p, 1);
            RET_CHECK_BLACKLIST(-1, ret, "libmysql_query_1col");
        }
        // 验证目标目录下是否存在该文件名
        sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d AND `filename` = '%s';", id_dir, filename);
        ret = libmysql_query_11count(program_stat->mysql_connect, query_str);
        check_flag = 0 == ret;
    }

    if (check_flag) {
        ret = msg_cs_cp_r(program_stat->mysql_connect, id_source, id_dir, filename);
        RET_CHECK_BLACKLIST(-1, ret, "msg_cs_cp_r");
        sendbuf.approve = APPROVE;
    }

    // 向客户端发送消息
    ret = msg_cs_cp_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cs_cp_send");

    return 0;
}

static int msg_cs_cp_r(MYSQL *mysql_connect, int id_source, int id_dir, const char *rename) {
    int ret = 0;
    char query_str[1024] = {0};

    // 先复制源文件/目录
    if (rename) {
        sprintf(query_str, "INSERT INTO `user_file`(`preid`, `type`, `filename`, `userid`, `filesize`, `md5`) SELECT @pre := %d, `type`, @fn := '%s', `userid`, `filesize`, `md5` FROM `user_file` WHERE `id` = %d;", id_dir, rename, id_source);
    } else {
        sprintf(query_str, "INSERT INTO `user_file`(`preid`, `type`, `filename`, `userid`, `filesize`, `md5`) SELECT @pre := %d, `type`, `filename`, `userid`, `filesize`, `md5` FROM `user_file` WHERE `id` = %d;", id_dir, id_source);
    }
    ret = mysql_query(mysql_connect, query_str);
    RET_CHECK_BLACKLIST(-1, ret, "mysql_query");

    // 查询源的类型与文件名
    char type_str[4] = {0};
    char filename[64] = {0};
    char *res_p[] = {&type_str[0], &filename[0]};
    sprintf(query_str, "SELECT `type`, `filename` FROM `user_file` WHERE `id` = %d;", id_source);
    ret = libmysql_query_1row(mysql_connect, query_str, res_p, 2);

    // 如果源为目录, 则需要进行递归
    if (0 == strcmp(type_str, "d")) {
        // 获取源目录下的文件数量
        sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d;", id_source);
        int filenum = libmysql_query_11count(mysql_connect, query_str);
        // 如果源目录下有文件存在
        if (filenum) {
            // 获取新的目标路径
            int id_dir_new = 0;
            if (rename) {
                sprintf(query_str, "SELECT `id` FROM `user_file` WHERE `preid` = %d AND `filename` = '%s';", id_dir, rename);
            } else {
                sprintf(query_str, "SELECT `id` FROM `user_file` WHERE `preid` = %d AND `filename` = '%s';", id_dir, filename);
            }
            printf("%s\n", query_str);
            id_dir_new = libmysql_query_11count(mysql_connect, query_str);
            // 申请空间
            char **fileid = (char **)malloc(sizeof(char *) * filenum);
            fileid[0] = (char *)malloc(16 * filenum);
            for (int i = 0; i < filenum; i++) {
                fileid[i] = fileid[0] + i * 16;
            }
            // 在数据库中查询
            sprintf(query_str, "SELECT `id` FROM `user_file` WHERE `preid` = %d;", id_source);
            ret = libmysql_query_1col(mysql_connect, query_str, fileid, filenum);
            RET_CHECK_BLACKLIST(ret, -1, "libmysql_query_1col");
            // 递归进行复制
            for (int i = 0; i < filenum; i++) {
                ret = msg_cs_cp_r(mysql_connect, atoi(fileid[i]), id_dir_new, NULL);
                RET_CHECK_BLACKLIST(-1, ret, "msg_cs_cp_r");
            }
            // 释放空间
            free(fileid[0]);
            free(fileid);
        }
    }
    return 0;
}