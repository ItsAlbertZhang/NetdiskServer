#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

int connect_msg_handle(struct connect_stat_t *connect_stat) {
    int ret = 0;
    char buf[1024] = {0};

    

    return 0;
}

// 循环接收, 避免因网络数据分包导致的错误
size_t recv_n(int connect_fd, void *buf, size_t len, int flags) {
    int ret = 0;
    char *p = (char *)buf;
    size_t recved_len = 0;
    while (recved_len < len) {
        ret = recv(connect_fd, p + recved_len, len - recved_len, flags);
        RET_CHECK_BLACKLIST(-1, ret, "recv");
        if (0 == ret) {
            return 0; // 当对端断开时, 返回 0.
        }
        recved_len += ret;
    }
    return recved_len; // 正常情况下, 返回接收到的字节数.
}