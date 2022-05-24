#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

struct msg_login_recvbuf_t {
    char msgtype;          // 消息类型
    int username_len;      // 下一字段的长度
    char username[30];     // 用户名
    int pwd_ciprsa_len;    // 下一字段的长度
    char pwd_ciprsa[1024]; // 密码密文
};

struct msg_login_sendbuf_t {
    char msgtype; // 消息类型
    char approve; // 批准标志
};

#define APPROVE 1
#define DISAPPROVE 0
#define USERNAME_NOT_EXIST -1
#define PWD_ERROR -2

static int msg_login_recv(int connect_fd, struct msg_login_recvbuf_t *recvbuf) {
    int ret = 0;

    bzero(recvbuf, sizeof(struct msg_login_recvbuf_t));
    ret = recv_n(connect_fd, &recvbuf->username_len, sizeof(recvbuf->username_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->username, recvbuf->username_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, &recvbuf->pwd_ciprsa_len, sizeof(recvbuf->pwd_ciprsa_len), 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");
    ret = recv_n(connect_fd, recvbuf->pwd_ciprsa, recvbuf->pwd_ciprsa_len, 0);
    RET_CHECK_BLACKLIST(-1, ret, "recv");

    return 0;
}

static int msg_login_send(int connect_fd, struct msg_login_sendbuf_t *sendbuf) {
    int ret = 0;

    sendbuf->msgtype = MT_LOGIN;
    ret = send(connect_fd, &sendbuf->msgtype, sizeof(sendbuf->msgtype), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");
    ret = send(connect_fd, &sendbuf->approve, sizeof(sendbuf->approve), MSG_NOSIGNAL);
    RET_CHECK_BLACKLIST(-1, ret, "send");

    return 0;
}

int msg_login(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat) {
    int ret = 0;

    // 准备资源
    struct msg_login_recvbuf_t recvbuf;
    struct msg_login_sendbuf_t sendbuf;
    bzero(&sendbuf, sizeof(sendbuf));
    bzero(&recvbuf, sizeof(recvbuf));

    // 接收来自客户端的消息
    ret = msg_login_recv(connect_stat->fd, &recvbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_regist_recv");

    // 查询数据库: 该用户名是否存在
    char query_str[1024] = {0};
    sprintf(query_str, "SELECT COUNT(*) FROM `user_auth` WHERE `username` = '%s';", recvbuf.username);
    int dupnum = libmysql_query_count(program_stat->mysql_connect, query_str);
    if (0 == dupnum) {
        sendbuf.approve = USERNAME_NOT_EXIST; // 用户名不存在
        sprintf(logbuf, "已拒绝 fd 为 %d 的用户名为 %s 的登录请求: 该用户名不存在.", connect_stat->fd, recvbuf.username);
        logging(LOG_INFO, logbuf);
    } else {
        // 对接收到的密文进行 rsa 解密处理
        char pwd_plaintext[1024] = {0};
        rsa_decrypt(pwd_plaintext, recvbuf.pwd_ciprsa, program_stat->private_rsa, PRIKEY); // 对密码进行 rsa 解密
        for (int i = 0; i < strlen(pwd_plaintext); i++) {
            pwd_plaintext[i] = pwd_plaintext[i] ^ connect_stat->token[i]; // 对密码进行 token 异或, 得到密码明文原文
        }
        // 数据库比对
        char pwd_ciphertext_sha512_mysql[128] = {0};
        char *pwd_ciphertext_sha512_mysql_p[] = {&pwd_ciphertext_sha512_mysql[0]};
        char query_str[1024] = {0};
        sprintf(query_str, "SELECT `pwd` FROM `user_auth` WHERE `username` = '%s';", recvbuf.username);
        ret = libmysql_query_1col(program_stat->mysql_connect, query_str, pwd_ciphertext_sha512_mysql_p, 1);
        if (1 != ret) {
            RET_CHECK_BLACKLIST(0, 0, "libmysql_query_1col");
        } else {
            // 成功获取数据库中存储的 pwd 信息
            // 获取盐值
            int cnt = 0, i = 0;
            for (i = 0; cnt < 3; i++) {
                cnt += pwd_ciphertext_sha512_mysql[i] == '$';
            }
            char salt[16] = {0};
            strncpy(salt, pwd_ciphertext_sha512_mysql, i);
            // 进行加密计算
            char pwd_ciphertext_sha512_cal[128] = {0};
            strcpy(pwd_ciphertext_sha512_cal, crypt(pwd_plaintext, salt));
            bzero(pwd_plaintext, sizeof(pwd_plaintext)); // 清空密码明文, 确保安全
            // 比对
            if (strcmp(pwd_ciphertext_sha512_mysql, pwd_ciphertext_sha512_cal)) { // 密码错误
                sendbuf.approve = PWD_ERROR;
                sprintf(logbuf, "已拒绝 fd 为 %d 的用户名为 %s 的登录请求: 密码错误.", connect_stat->fd, recvbuf.username);
                logging(LOG_INFO, logbuf);
            } else { // 密码正确, 成功登录
                sendbuf.approve = APPROVE;
                sprintf(logbuf, "已通过 fd 为 %d 的用户名为 %s 的登录请求.", connect_stat->fd, recvbuf.username);
                logging(LOG_INFO, logbuf);
                // 获取用户 userid
                char userid_s[10] = {0};
                char *userid_sp[] = {&userid_s[0]};
                char query_str[1024] = {0};
                sprintf(query_str, "SELECT `userid` FROM `user_auth` WHERE `username` = '%s';", recvbuf.username);
                ret = libmysql_query_1col(program_stat->mysql_connect, query_str, userid_sp, 1);
                if (1 != ret) {
                    RET_CHECK_BLACKLIST(0, 0, "libmysql_query_1col");
                } else {
                    connect_stat->userid = atoi(userid_s);
                    connect_stat->pwd_id = 0;
                    sprintf(logbuf, "成功获取用户名为 %s 的 userid 为 %d.", recvbuf.username, connect_stat->userid);
                    logging(LOG_DEBUG, logbuf);
                }
            }
        }
    }

    // 向客户端发送消息
    ret = msg_login_send(connect_stat->fd, &sendbuf);
    RET_CHECK_BLACKLIST(-1, ret, "msg_login_send");

    return 0;
}