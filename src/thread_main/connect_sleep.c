#include "head.h"
#include "thread_main.h"
#include "mylibrary.h"

struct connect_sleep_node *connect_sleep_init(void) {
    struct connect_sleep_node *headnode = (struct connect_sleep_node *)malloc(sizeof(struct connect_sleep_node));
    headnode->data.len = 0;
    headnode->next = NULL;
    return headnode;
}

void connect_sleep_destory(struct connect_sleep_node *headnode) {
    if (headnode->next) {
        connect_sleep_destory(headnode->next);
    }
    free(headnode);
}

int connect_sleep_fall(struct connect_sleep_node *headnode, struct connect_stat_t *connect_stat) {
    struct connect_sleep_node *newnode = (struct connect_sleep_node *)malloc(sizeof(struct connect_sleep_node));

    memcpy(&newnode->data.conn.addr, &connect_stat->addr, sizeof(newnode->data.conn.addr));
    memcpy(newnode->data.conn.confirm, connect_stat->confirm, sizeof(newnode->data.conn.confirm));
    newnode->data.conn.init_time = connect_stat->init_time;
    newnode->data.conn.user_id = connect_stat->user_id;
    newnode->data.conn.pwd_id = connect_stat->pwd_id;

    newnode->next = headnode->next;
    headnode->next = newnode;
    headnode->data.len += 1;
    bzero(connect_stat, sizeof(struct connect_stat_t));
    sprintf(logbuf, "已将初次连接时间为 %ld 的连接放入休眠链表, 其确认码为 %s, 用户为 %d, 当前工作目录为 %d.",  newnode->data.conn.init_time, newnode->data.conn.confirm, newnode->data.conn.user_id, newnode->data.conn.pwd_id);
    logging(LOG_DEBUG, logbuf);
    return 0;
}

int connect_sleep_awake(struct connect_sleep_node *headnode, time_t init_time, struct connect_stat_t *connect_stat) {
    int ret = -1;
    struct connect_sleep_node *thisnode = headnode;
    while (-1 == ret && thisnode->next) {
        if (thisnode->next->data.conn.init_time == init_time) {
            struct connect_sleep_node *awakenode = thisnode->next;
            if (!memcmp(&connect_stat->addr.sin_addr, &awakenode->data.conn.addr.sin_addr, sizeof(connect_stat->addr.sin_addr))) {
                memcpy(connect_stat->confirm, awakenode->data.conn.confirm, sizeof(connect_stat->confirm));
                connect_stat->init_time = awakenode->data.conn.init_time;
                connect_stat->user_id = awakenode->data.conn.user_id;
                connect_stat->pwd_id = awakenode->data.conn.pwd_id;

                thisnode->next = awakenode->next;
                headnode->data.len -= 1;
                free(awakenode);
                ret = 0;
            }
        }
        thisnode = thisnode->next;
    }
    return ret;
}