#include "head.h"

int epoll_add(int epfd, int fd) {
    int ret = 0;
    struct epoll_event event;

    bzero(&event, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = fd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    RET_CHECK_BLACKLIST(-1, ret, "epoll_ctl");

    return 0;
}

int epoll_del(int epfd, int fd) {
    int ret = 0;

    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    RET_CHECK_BLACKLIST(-1, ret, "epoll_ctl");

    return 0;
}