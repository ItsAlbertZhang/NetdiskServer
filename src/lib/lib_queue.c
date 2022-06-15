#include "head.h"
#include "mylibrary.h"

int queue_init(struct queue_t **pQ, int len) {
    *pQ = (struct queue_t *)malloc(sizeof(struct queue_t));
    struct queue_t *Q = *pQ;
    bzero(Q, sizeof(struct queue_t));
    Q->elem_array = (struct queue_elem_t *)malloc(sizeof(struct queue_elem_t) * len);
    bzero(Q->elem_array, sizeof(struct queue_elem_t) * len);
    Q->len = len;
    return 0;
}

int queue_destroy(struct queue_t **pQ) {
    free((*pQ)->elem_array); // 释放队列数组
    free((*pQ));             // 释放队列
    *pQ = NULL;              // 将队列指针指向空
    return 0;
}

int queue_in(struct queue_t *Q, struct queue_elem_t elem) {
    if (Q->front == Q->rear && Q->tag != 0) {
        // 队满
        return -1;
    } else {
        memcpy(&Q->elem_array[Q->rear], &elem, sizeof(struct queue_elem_t));
        Q->rear = (Q->rear + 1) % Q->len;
        Q->tag = 1;
    }
    return 0;
}

int queue_out(struct queue_t *Q, struct queue_elem_t *elem) {
    if (Q->front == Q->rear && Q->tag == 0) {
        // 队空
        return -1;
    } else {
        memcpy(elem, &Q->elem_array[Q->front], sizeof(struct queue_elem_t));
        Q->front = (Q->front + 1) % Q->len;
        Q->tag = 0;
    }
    return 0;
}