#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_regist_recvbuf_t {
    char msgtype;              // 消息类型
    int username_len;          // 下一字段的长度
    char username[30];         // 用户名
    int pwd_len;               // 下一字段的长度
    char pwd_ciphertext[1024]; // 密码密文
};

struct msg_regist_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0

static int msg_regist_recv(int connect_fd, struct msg_regist_recvbuf_t *recvbuf) {
    int ret = 0;

    bzero(recvbuf, sizeof(struct msg_regist_recvbuf_t));
    ret = recv_n(connect_fd, &recvbuf->username_len, sizeof(recvbuf->username_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->username, recvbuf->username_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->pwd_len, sizeof(recvbuf->pwd_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->pwd_ciphertext, recvbuf->pwd_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_regist_send(int connect_fd, struct msg_regist_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_REGIST;
    ret = send(connect_fd, sendbuf, sizeof(struct msg_regist_sendbuf_t), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

int msg_regist(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_regist_recvbuf_t recvbuf;
    struct msg_regist_sendbuf_t sendbuf;
    bzero(&sendbuf, sizeof(sendbuf));
    bzero(&recvbuf, sizeof(recvbuf));

    // 接收来自客户端的消息
    ret = msg_regist_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_recv");

    // 查询数据库: 该用户名是否已存在
    int dupnum = libmysql_dupnum_value(program_stat->mysql_connect, "user_auth", "username", recvbuf.username);
    if (0 == dupnum) { // 用户名未被注册
        sendbuf.approve = APPROVE;
        // 如果是正式注册请求, 则应对密码段进行处理
        if (recvbuf.pwd_len) {
            // 对接收到的密文进行 rsa 解密处理
            char pwd_plaintext[32] = {0};
            rsa_decrypt(pwd_plaintext, recvbuf.pwd_ciphertext, program_stat->private_rsa, PRIKEY); // 对密码进行 rsa 解密
            for (int i = 0; i < strlen(pwd_plaintext); i++) {
                pwd_plaintext[i] = pwd_plaintext[i] ^ connect_stat->confirm[i]; // 对密码进行确认码异或, 得到密码明文原文
            }
            // 对解密得到的明文进行 SHA512 加密处理
            char salt[16] = "$6$";                                    // 盐值的 id 为6, 代表采用 SHA512 算法
            random_gen_str(&salt[strlen(salt)], 8, connect_stat->fd); // 生成 8 个字节长的随机盐值
            /* crypt 函数需要使用的定义 #define _XOPEN_SOURCE 会使 ftruncate 函数与 readlink 函数不可用. 因此此处使用其可重入版本 crypt_r, 该版本需要使用定义 #define _GNU_SOURCE*/
            struct crypt_data pwd_ciphertext_data;
            bzero(&pwd_ciphertext_data, sizeof(pwd_ciphertext_data));
            char *pwd_ciphertext = crypt_r(pwd_plaintext, salt, &pwd_ciphertext_data);
            bzero(pwd_plaintext, sizeof(pwd_plaintext)); // 清空密码明文, 确保安全
            // 将用户信息插入 MySQL 数据库
            char query[1024] = {0};
            sprintf(query, "INSERT INTO user_auth(username, pwd) VALUES('%s', '%s');", recvbuf.username, pwd_ciphertext);
            // printf("%s\n", query);
            ret = mysql_query(program_stat->mysql_connect, query);
            RET_CHECK_BLACKLIST(-1, ret, "mysql_query");
            // printf("affected rows: %ld.\n", (long)mysql_affected_rows(program_stat->mysql_connect));
            // 日志
            sprintf(logbuf, "已接受 fd 为 %d 的用户名为 %s 的注册请求.", connect_stat->fd, recvbuf.username);
            logging(LOG_INFO, logbuf);
            // 获取用户 user_id
            char user_id_str[10] = {0};
            char *user_id_p[] = {&user_id_str[0]};
            ret = libmysql_query_1col(program_stat->mysql_connect, "user_auth", "user_id", "username", recvbuf.username, user_id_p, 1);
            if (1 != ret) {
                RET_CHECK_BLACKLIST(0, 0, "libmysql_query_1col");
            } else {
                connect_stat->user_id = atoi(user_id_str);
                connect_stat->pwd_id = 0;
                sprintf(logbuf, "成功获取用户名为 %s 的 user_id 为 %d.", recvbuf.username, connect_stat->user_id);
                logging(LOG_DEBUG, logbuf);
            }
        }
    } else { // 用户名已存在, 不能注册
        sendbuf.approve = DISAPPROVE;
        sprintf(logbuf, "已拒绝 fd 为 %d 的用户名为 %s 的注册请求.", connect_stat->fd, recvbuf.username);
        logging(LOG_INFO, logbuf);
    }

    // 向客户端发送消息
    ret = msg_regist_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_send");

    return 0;
}