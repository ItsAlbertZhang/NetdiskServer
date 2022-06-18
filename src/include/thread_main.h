#ifndef __THREAD_MAIN_H__
#define __THREAD_MAIN_H__

#include "head.h"
#include "main.h"

#define AUTO_DISCONNECT_SECOND 16 // 16 秒未操作自动断开

// 现有连接状态
struct connect_stat_t {
    int fd;                  // 存放连接的真实文件描述符. 如果为 0 说明尚未存放连接. 初次赋值于 connect_init_handle.
    struct sockaddr_in addr; // 存放连接对端的地址信息. 初次赋值于 connect_init_handle.
    int connect_timer_index; // 连接所在的计时器位置下标. 初次赋值于 connect_timer_in.
    int connect_timer_real;  // 连接应在的计时器位置下标. 初次赋值于 connect_timer_in.
    char token[32];          // 连接的 token. 初次赋值于 msg_conninit.
    int userid;              // 用户 id. 初次赋值于 msg_login 或 msg_regist.
    int pwd_id;              // 当前工作目录 id. 初次赋值于 msg_login 或 msg_regist.
};

// 时间轮定时器: 哈希表结点数据内容
union connect_timer_hashnode_data {
    struct connect_stat_t *conn; // 非头结点, 指向实际数据
    long len;                    // 头结点, 记录拉链长度
};

// 时间轮定时器: 哈希表结点
struct connect_timer_hashnode {
    union connect_timer_hashnode_data data;
    struct connect_timer_hashnode *next;
};

// 休眠连接状态
struct connect_sleep_t {
    struct sockaddr_in addr; // 存放连接对端的地址信息
    char token[64];          // 连接的 token
    int userid;              // 用户 id
    int pwd_id;              // 当前工作目录 id
};

// 休眠连接链表数据内容
union connect_sleep_nodedata {
    struct connect_sleep_t conn;
    int len;
};

// 休眠连接链表
struct connect_sleep_node {
    union connect_sleep_nodedata data;
    struct connect_sleep_node *next;
};

extern int epfd;

// 消息来源为已有连接
int connect_msg_handle(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr, struct program_stat_t *program_stat, struct connect_sleep_node *connect_sleep);

// 消息来源为 socket_fd, 有新连接
int connect_init_handle(int socket_fd, struct connect_stat_t *connect_stat_arr, int max_connect_num, struct connect_timer_hashnode *connect_timer_arr);

// 销毁连接
int connect_destory(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr, struct connect_sleep_node *connect_sleep, int gotosleep);

// 将新连接放入时间轮定时器
int connect_timer_in(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 将连接从时间轮定时器中取出
int connect_timer_out(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 移动时间轮定时器中的连接至其应在位置
int connect_timer_move(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 处理下一秒对应的时间轮片上的连接
int connect_timer_handle(struct connect_timer_hashnode *connect_timer_arr, struct connect_sleep_node *connect_sleep);

// 将连接应在的位置更新至连接状态
int connect_timer_update(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 初始化休眠连接链表
struct connect_sleep_node *connect_sleep_init(void);

// 销毁休眠连接链表
void connect_sleep_destory(struct connect_sleep_node *headnode);

// 将现有连接中的信息添加入休眠连接链表
int connect_sleep_fall(struct connect_sleep_node *headnode, struct connect_stat_t *connect_stat);

// 取出休眠连接链表中的信息并填入现有连接
int connect_sleep_awake(struct connect_sleep_node *headnode, char *token_plain, struct connect_stat_t *connect_stat);

#endif