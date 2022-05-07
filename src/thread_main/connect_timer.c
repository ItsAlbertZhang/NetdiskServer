#include "head.h"
#include "program_stat.h"
#include "thread_main.h"

int connect_timer_in(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr, const int *index) {
    int i;
    if (index) {
        i = *index;
    } else {
        time_t now = time(NULL);
        i = now % AUTO_DISCONNECT_SECOND; // 新连接应该在的计时器位置
    }
    // 头插法插入新拉链节点
    struct connect_timer_hashnode *newnode = (struct connect_timer_hashnode *)malloc(sizeof(struct connect_timer_hashnode));
    newnode->data.conn = connect_stat;
    newnode->next = connect_timer_arr[i].next;
    connect_timer_arr[i].next = newnode;
    // 拉链长度 + 1
    connect_timer_arr[i].data.len += 1;
    // 将计时器信息存入连接状态
    connect_stat->connect_timer_index = i;
    connect_stat->connect_timer_real = 0;

    return 0;
}

int connect_timer_out(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr) {
    // 删除结点
    struct connect_timer_hashnode *prenode = &connect_timer_arr[connect_stat->connect_timer_index];
    while (prenode->next->data.conn != connect_stat) {
        prenode = prenode->next;
        if (NULL == prenode) {
            return -1;
        }
    }
    struct connect_timer_hashnode *thisnode = prenode->next;
    prenode->next = thisnode->next;
    free(thisnode);
    // 拉链长度 - 1
    connect_timer_arr[connect_stat->connect_timer_index].data.len -= 1;

    return 0;
}

int connect_timer_handle_next_second(struct connect_timer_hashnode *connect_timer_arr) {
    int ret = 0;
    time_t now = time(NULL) + 1;          // 获取距离上一次操作已过 29 秒的连接
    int i = now % AUTO_DISCONNECT_SECOND; // 要断开的连接应该在的计时器位置
    // 寻找连接
    struct connect_timer_hashnode *thisnode;
    struct connect_stat_t *thisstat;
    int index;
    while (connect_timer_arr[i].next) {
        thisnode = connect_timer_arr[i].next;
        thisstat = thisnode->data.conn;
        if (thisnode->data.conn->connect_timer_real) {
            // 说明在连接之后, 这个连接又有了活动. 此时需要更新连接状态, 并将其放至其应在的头结点下.
            index = thisstat->connect_timer_real;
            ret = connect_timer_out(thisstat, connect_timer_arr);
            RET_CHECK_BLACKLIST(-1, ret, "connect_timer_out");
            ret = connect_timer_in(thisstat, connect_timer_arr, &index);
        } else {
            // 说明该节点已至少 29 秒无活动
            ret = connect_timer_out(thisstat, connect_timer_arr); // 释放结点
            RET_CHECK_BLACKLIST(-1, ret, "connect_timer_out");
            close(thisstat->fd); // 关闭连接
            RET_CHECK_BLACKLIST(-1, ret, "close");
            bzero(thisstat, sizeof(struct connect_stat_t)); // 清空状态
        }
    }
}