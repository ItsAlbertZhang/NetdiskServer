#include "head.h"
#include "thread_child.h"

int child_s2c(int connect_fd, int filefd, size_t filesize) {
    int ret = 0;
    ret = sendfile(connect_fd, filefd, NULL, filesize);
    return 0;
}