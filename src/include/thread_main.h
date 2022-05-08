#ifndef __THREAD_MAIN_H__
#define __THREAD_MAIN_H__

#include "head.h"

#define AUTO_DISCONNECT_SECOND 16 // 16 秒未操作自动断开

// 连接状态
struct connect_stat_t {
    int fd;                  // 存放连接的真实文件描述符. 如果为 0 说明尚未存放连接.
    struct sockaddr_in addr; // 存放连接对端的地址信息
    int connect_timer_index; // 连接所在的计时器位置下标
    int connect_timer_real;  // 连接应在的计时器位置下标
};

// 哈希表结点数据部分
union connect_timer_hashnode_data {
    struct connect_stat_t *conn; // 非头结点, 指向实际数据
    long len;                    // 头结点, 记录拉链长度
};

// 哈希表结点
struct connect_timer_hashnode {
    union connect_timer_hashnode_data data;
    struct connect_timer_hashnode *next;
};

// 未实现
int epoll_handle_established(void);

// 消息来源为 socket_fd, 有新连接
int epoll_handle_socket(int socket_fd, struct connect_stat_t *connect_stat_arr, int max_connect_num, struct connect_timer_hashnode *connect_timer_arr);

// 将新连接放入时间轮定时器
int connect_timer_in(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 将连接从时间轮定时器中取出
int connect_timer_out(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 移动时间轮定时器中的连接至其应在位置
int connect_timer_move(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 处理下一秒对应的时间轮片上的连接
int connect_timer_handle_next_second(struct connect_timer_hashnode *connect_timer_arr);

#endif