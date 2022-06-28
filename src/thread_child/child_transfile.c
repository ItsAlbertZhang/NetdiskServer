#include "head.h"
#include "thread_child.h"

int child_s2c(int connect_fd, int filefd, struct progress_t *progress_bar) {
    int ret = 0;
    // 将 connect_fd 改为非阻塞
    int connectfd_cntl = fcntl(connect_fd, F_GETFL);
    connectfd_cntl |= O_NONBLOCK;
    ret = fcntl(connect_fd, F_SETFL, connectfd_cntl);
    RET_CHECK_BLACKLIST(-1, ret, "fcntl");
    while (progress_bar->completedsize < progress_bar->filesize) {
        ret = sendfile(connect_fd, filefd, NULL, progress_bar->filesize - progress_bar->completedsize);
        if (ret > 0) {
            progress_bar->completedsize += ret;
        }
    }
    // 恢复 connect_fd 为阻塞
    connectfd_cntl &= ~O_NONBLOCK;
    ret = fcntl(connect_fd, F_SETFL, connectfd_cntl);
    RET_CHECK_BLACKLIST(-1, ret, "fcntl");
    // ret = sendfile(connect_fd, filefd, NULL, progress_bar->filesize);
    // printf("ret = %d\n", ret);
    return 0;
}