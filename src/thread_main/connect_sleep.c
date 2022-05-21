#include "head.h"
#include "mylibrary.h"
#include "thread_main.h"

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
    memcpy(newnode->data.conn.token, connect_stat->token, sizeof(newnode->data.conn.token));
    newnode->data.conn.userid = connect_stat->userid;
    newnode->data.conn.pwd_id = connect_stat->pwd_id;

    newnode->next = headnode->next;
    headnode->next = newnode;
    headnode->data.len += 1;
    sprintf(logbuf, "已将 fd 为 %d 的连接放入休眠链表, 其 token 为 %s, 用户为 %d, 当前工作目录为 %d.", connect_stat->fd, newnode->data.conn.token, newnode->data.conn.userid, newnode->data.conn.pwd_id);
    logging(LOG_DEBUG, logbuf);

    bzero(connect_stat, sizeof(struct connect_stat_t));
    return 0;
}

int connect_sleep_awake(struct connect_sleep_node *headnode, char *token_plain, struct connect_stat_t *connect_stat) {
    int ret = -1;
    struct connect_sleep_node *thisnode = headnode;
    while (-1 == ret && thisnode->next) {
        if (!strcmp(token_plain, thisnode->next->data.conn.token)) {
            struct connect_sleep_node *awakenode = thisnode->next;
            if (!memcmp(&connect_stat->addr.sin_addr, &awakenode->data.conn.addr.sin_addr, sizeof(struct in_addr))) {
                memcpy(connect_stat->token, awakenode->data.conn.token, sizeof(connect_stat->token));
                connect_stat->userid = awakenode->data.conn.userid;
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