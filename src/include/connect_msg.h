#ifndef __CONNECT_MSG_H__
#define __CONNECT_MSG_H__

#include "head.h"
#include "main.h"
#include "thread_main.h"

// 消息类型标志
enum msg_type {
    MT_NULL,     // 编号 0 为空请求
    MT_CONNINIT, // 下发验证请求
    MT_REGIST,   // 注册请求
    MT_LOGIN,    // 登录请求
    MT_DUPCONN,  // 拷贝连接请求
    MT_CS_PWD,   // 短命令(command short): pwd
    MT_CS_LS,    // 短命令(command short): ls
    MT_CS_CD,    // 短命令(command short): cd
    MT_COMM_S,   // 短命令请求
    MT_COMM_L,   // 长命令请求
};

// 循环接收, 避免因网络数据分包导致的错误
size_t recv_n(int connect_fd, void *buf, size_t len, int flags);

// 接收来自客户端的消息类型标志
int connect_msg_fetchtype(int connect_fd, void *buf);

// 下发验证请求
int msg_conninit(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat);

// 注册请求
int msg_regist(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat);

// 登录请求
int msg_login(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat);

// 拷贝连接请求
int msg_dupconn(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat, struct connect_sleep_node *connect_sleep);

// pwd 命令请求
int msg_cs_pwd(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat);

// ls 命令请求
int msg_cs_ls(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat);

// cd 命令请求
int msg_cs_cd(struct connect_stat_t *connect_stat, struct program_stat_t *program_stat);

#endif /* __CONNECT_MSG_H__ */