#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_cl_s2c_recvbuf_t {
    char msgtype;   // 消息类型
    int dir_len;    // 下一字段的长度
    char dir[1024]; // 原文件或目录
};

struct msg_cl_s2c_sendbuf_t {
    char msgtype;
    size_t filesize;
    int filename_len;
    char filename[64];
};

static int msg_cl_s2c_recv(int connect_fd, struct msg_cl_s2c_recvbuf_t *recvbuf) {
    int ret = 0;

    ret = recv_n(connect_fd, &recvbuf->dir_len, sizeof(recvbuf->dir_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->dir, recvbuf->dir_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_cl_s2c_send(int connect_fd, struct msg_cl_s2c_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_CL_S2C;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->filesize, sizeof(sendbuf->filesize), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->filename_len, sizeof(sendbuf->filename_len), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, sendbuf->filename, sendbuf->filename_len, MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    printf("msgtype = %d, filesize = %ld, filenamelen = %d\n", sendbuf->msgtype, sendbuf->filesize, sendbuf->filename_len);

    return 0;
}

int msg_cl_s2c(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat, struct connect_timer_hashnode *connect_timer_arr) {
    int ret = 0;

    // 准备资源
    struct msg_cl_s2c_recvbuf_t recvbuf;
    struct msg_cl_s2c_sendbuf_t sendbuf;
    bzero(&recvbuf, sizeof(recvbuf));
    bzero(&sendbuf, sizeof(sendbuf));

    // 接收来自客户端的消息
    ret = msg_cl_s2c_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_cl_s2c_recv");

    // 获取文件 id
    int id_file = connect_stat->pwd_id;
    ret = msg_lib_path2id(recvbuf.dir, &id_file, program_stat->mysql_connect);

    // 如果文件的类型是 FILE, 则允许继续执行
    if (TYPE_FILE == ret) {
        // 获取文件名, 文件大小与文件 MD5 码, 文件名与文件大小用于发送至客户端, MD5 码用于发送给子线程.
        char query_str[1024] = {0};
        char filename[64] = {0};
        char filesize[64] = {0};
        char filemd5[64] = {0};
        char *res_p[] = {&filename[0], &filesize[0], &filemd5[0]};
        sprintf(query_str, "SELECT `filename`, `filesize`, `md5` FROM `user_file` WHERE `id` = %d;", id_file);
        ret = libmysql_query_1row(program_stat->mysql_connect, query_str, res_p, 3);
        RET_CHECK_BLACKLIST(-1, ret, "libmysql_query_1col");

        // 入队. (队列为线程资源队列, 用于存放待子线程处理的请求.)
        struct queue_elem_t elem;
        elem.flag = QUEUE_FLAG_S2C;
        elem.connect_fd = connect_stat->fd;
        strcpy(elem.file_md5, filemd5);
        elem.filesize = atol(filesize);
        ret = queue_in(program_stat->thread_stat.thread_resource.queue, &elem);

        // 只有当入队成功时, 才执行以下步骤
        if (-1 != ret) {
            sendbuf.filesize = atol(filesize);
            strcpy(sendbuf.filename, filename);
            sendbuf.filename_len = strlen(sendbuf.filename);
            printf("msgtype = %d, filesize = %ld, filenamelen = %d\n", sendbuf.msgtype, sendbuf.filesize, sendbuf.filename_len);
            // 向客户端发送消息
            ret = msg_cl_s2c_send(connect_stat->fd, &sendbuf);
            RET_CHECK_BLACKLIST(-1, ret, "msg_cl_s2c_send");
            // 将连接从时间轮定时器上取出
            ret = connect_timer_out(connect_stat, connect_timer_arr);
            RET_CHECK_BLACKLIST(-1, ret, "connect_timer_out");
            // 向子线程发送委派请求
            pthread_cond_signal(&program_stat->thread_stat.thread_resource.cond);
            logging(LOG_DEBUG, "已向子线程发出委派请求.");
        }
    }

    return 0;
}