#include "head.h"
#include "thread_child.h"

int child_s2c(int connect_fd, int filefd, struct progress_bar_t progress_bar) {
    int ret = 0;
    ret = sendfile(connect_fd, filefd, NULL, progress_bar.filesize);
    return 0;
}