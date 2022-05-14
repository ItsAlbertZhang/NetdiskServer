#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

int connect_timer_in(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr) {
    time_t now = time(NULL);
    int i = now % AUTO_DISCONNECT_SECOND; // 放入的时间轮片
    // 头插法插入新拉链节点
    struct connect_timer_hashnode *newnode = (struct connect_timer_hashnode *)malloc(sizeof(struct connect_timer_hashnode));
    newnode->data.conn = connect_stat;
    newnode->next = connect_timer_arr[i].next;
    connect_timer_arr[i].next = newnode;
    // 拉链长度 + 1
    connect_timer_arr[i].data.len += 1;
    // 将定时器信息存入连接状态
    connect_stat->connect_timer_index = i;
    connect_stat->connect_timer_real = -1;

    return 0;
}

int connect_timer_out(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr) {
    // 获取连接所在的时间轮定时器结点和其前驱结点
    struct connect_timer_hashnode *prenode = &connect_timer_arr[connect_stat->connect_timer_index];
    while (prenode->next->data.conn != connect_stat) {
        prenode = prenode->next;
        if (NULL == prenode) {
            return -1;
        }
    }
    struct connect_timer_hashnode *thisnode = prenode->next;
    // 将连接所在的结点移开
    prenode->next = thisnode->next;
    connect_timer_arr[connect_stat->connect_timer_index].data.len -= 1;
    // 删除结点并释放内存
    free(thisnode);

    return 0;
}

int connect_timer_move(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr) {
    // 获取连接所在的时间轮定时器结点和其前驱结点
    struct connect_timer_hashnode *prenode = &connect_timer_arr[connect_stat->connect_timer_index];
    while (prenode->next->data.conn != connect_stat) {
        prenode = prenode->next;
        if (NULL == prenode) {
            return -1;
        }
    }
    struct connect_timer_hashnode *thisnode = prenode->next;
    // 将连接所在的结点移开
    prenode->next = thisnode->next;
    connect_timer_arr[connect_stat->connect_timer_index].data.len -= 1;
    // 将连接所在的结点插入到其应在的结点
    thisnode->next = connect_timer_arr[connect_stat->connect_timer_real].next;
    connect_timer_arr[connect_stat->connect_timer_real].next = thisnode;
    connect_timer_arr[connect_stat->connect_timer_real].data.len += 1;
    // 将定时器信息存入连接状态
    connect_stat->connect_timer_index = connect_stat->connect_timer_real;
    connect_stat->connect_timer_real = -1;

    sprintf(logbuf, "已检查 %d 号连接, 并成功将其移动至其应在结点.", connect_stat->fd);
    logging(LOG_DEBUG, logbuf);

    return 0;
}

int connect_timer_handle(struct connect_timer_hashnode *connect_timer_arr) {
    int ret = 0;
    time_t now = time(NULL) + 1;
    int i = now % AUTO_DISCONNECT_SECOND; // 获取下一秒对应的时间轮片
    // 获取时间轮片上的连接
    struct connect_timer_hashnode *thisnode;
    struct connect_stat_t *thisstat;
    while (connect_timer_arr[i].next) {
        // 只要该时间轮片上依然存在连接, 就继续循环
        thisnode = connect_timer_arr[i].next;
        thisstat = thisnode->data.conn;
        if (-1 == thisnode->data.conn->connect_timer_real) {
            // 该连接在上一个时间轮循环中无活动
            ret = connect_destory(thisstat, connect_timer_arr);
            RET_CHECK_BLACKLIST(-1, ret, "connect_destory");
        } else {
            // 该连接在上一个时间轮循环中有活动
            ret = connect_timer_move(thisstat, connect_timer_arr); // 移动时间轮定时器中的连接至其应在位置
            RET_CHECK_BLACKLIST(-1, ret, "connect_timer_move");
        }
    }
}

int connect_timer_update(struct connect_stat_t *connect_stat, struct connect_timer_hashnode *connect_timer_arr) {
    time_t now = time(NULL);
    if (connect_stat->fd) {
        connect_stat->connect_timer_real = now % AUTO_DISCONNECT_SECOND; // 放入的时间轮片
    }
    return 0;
}
