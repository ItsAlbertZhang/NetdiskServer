#include "head.h"
#include "program_init.h"

int init_tcp(char *local_sign, int max_listen_num, const char *config_dir, char config[][MAX_CONFIG_LENGTH]) {
    int ret = 0;

    // 获取 tcp 配置
    ret = getconfig(config_dir, "tcp.config", config);
    RET_CHECK_BLACKLIST(-1, ret, "getconfig");
    strcpy(local_sign, config[2]);

    // socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    RET_CHECK_BLACKLIST(-1, sockfd, "socket");

    // bind 前准备
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config[0]);
    server_addr.sin_port = htons(atoi(config[1]));
    // 设置端口重用
    int reuseaddr = 1;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
    RET_CHECK_BLACKLIST(-1, sockfd, "setsockopt");
    // 设置超时断连
    struct timeval tv;
    tv.tv_sec = 3; // recv 时 3 秒无响应返回 -1
    tv.tv_usec = 0;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(tv));
    // 执行 bind
    ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    RET_CHECK_BLACKLIST(-1, sockfd, "bind");

    ret = listen(sockfd, max_listen_num); // 执行 listen
    RET_CHECK_BLACKLIST(-1, sockfd, "listen");

    return sockfd;
}