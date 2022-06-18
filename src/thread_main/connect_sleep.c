#include "head.h"
#include "mylibrary.h"
#include "thread_main.h"

int connect_sleep_fall(struct queue_t *connect_sleep_queue, struct connect_stat_t *connect_stat) {
    struct connect_sleep_queue_elem_t elem;

    memcpy(&elem.addr, &connect_stat->addr, sizeof(elem.addr));
    memcpy(elem.token, connect_stat->token, sizeof(elem.token));
    elem.userid = connect_stat->userid;
    elem.pwd_id = connect_stat->pwd_id;

    // 队满处理
    if (connect_sleep_queue->front == connect_sleep_queue->rear && connect_sleep_queue->tag != 0) {
        struct connect_sleep_queue_elem_t elem_temp;
        queue_out(connect_sleep_queue, &elem_temp);
        logging(LOG_DEBUG, "休眠队列已满, 已将休眠时间最久的一条连接的信息丢弃.");
    }
    queue_in(connect_sleep_queue, &elem);

    sprintf(logbuf, "已将 fd 为 %d 的连接放入休眠链表, 其 token 为 %s, 用户为 %d, 当前工作目录为 %d.", connect_stat->fd, elem.token, elem.userid, elem.pwd_id);
    logging(LOG_DEBUG, logbuf);

    bzero(connect_stat, sizeof(struct connect_stat_t));
    return 0;
}

int connect_sleep_awake(struct queue_t *connect_sleep_queue, char *token_plain, struct connect_stat_t *connect_stat) {
    int ret = -1, i = 0;
    struct connect_sleep_queue_elem_t *arr = (struct connect_sleep_queue_elem_t *)connect_sleep_queue->elem_array;
    for (i = 0; i < connect_sleep_queue->len; i++) {
        if (!strcmp(token_plain, arr[i].token) && !memcmp(&connect_stat->addr.sin_addr, &arr[i].addr.sin_addr, sizeof(struct in_addr))) {
            memcpy(connect_stat->token, arr[i].token, sizeof(connect_stat->token));
            connect_stat->userid = arr[i].userid;
            connect_stat->pwd_id = arr[i].pwd_id;

            struct connect_sleep_queue_elem_t elem;
            queue_out(connect_sleep_queue, &elem);
            memcpy(&arr[i], &elem, sizeof(struct connect_sleep_queue_elem_t));

            ret = 0;
        }
    }
    return ret;
}