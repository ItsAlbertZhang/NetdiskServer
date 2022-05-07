#ifndef __THREAD_MAIN_H__
#define __THREAD_MAIN_H__

#include "head.h"

#define AUTO_DISCONNECT_SECOND 16 // 16 秒未操作自动断开

// 连接状态
struct connect_stat_t {
    int fd;                  // 存放连接的真实文件描述符. 如果为 0 说明尚未存放连接.
    struct sockaddr_in addr; // 存放连接对端的地址信息
    int connect_timer_index; // 连接计时器下标
    int connect_timer_real;  // 应在计时器位置
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

// 将连接加入计时器, 挂至指定结点. 如果 index 为 NULL, 则挂至当前时间对应结点, 否则挂至第 *index 个结点.
int connect_timer_in(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr, const int *index);

// 将连接从计时器中取出, 并销毁结点.
int connect_timer_out(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr);

// 处理下一秒对应的节点上, 仍然挂载着的结点.
int connect_timer_handle_next_second(struct connect_timer_hashnode *connect_timer_arr);

#endif