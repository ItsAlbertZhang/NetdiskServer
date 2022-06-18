#ifndef __THREAD_CHILD_H__
#define __THREAD_CHILD_H__

#include "head.h"

// 子线程处理函数
void *thread_child_handle(void *args);

// 子线程函数: server to client
int child_s2c(int connect_fd, int filefd, size_t filesize);

#endif