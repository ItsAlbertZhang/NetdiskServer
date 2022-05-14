#include "program_init.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"

int program_init(struct program_stat_t *program_stat) {
    int ret = 0;

    // 获取配置文件目录
    char config_dir[1024];
    ret = getconfig_init(config_dir, sizeof(config_dir));
    printf("成功获取配置文件目录.\n");

    char config[MAX_CONFIG_ROWS][MAX_CONFIG_LENGTH];

    // 初始化并获取 rsa 密钥
    ret = init_rsa_keys(&program_stat->private_rsa, &program_stat->public_rsa, config_dir);
    RET_CHECK_BLACKLIST(-1, ret, "init_rsa_keys");
    printf("成功获取 rsa 密钥.\n");

    // 初始化 MySQL 数据库连接
    ret = init_mysql(&program_stat->mysql_connect, config_dir, program_stat->private_rsa, program_stat->public_rsa, config);
    RET_CHECK_BLACKLIST(-1, ret, "init_mysql");
    printf("成功连接 MySQL 数据库.\n");

    // 初始化线程池
    ret = init_pthread_pool(&program_stat->thread_stat, config_dir, config);
    RET_CHECK_BLACKLIST(-1, ret, "init_pthread_pool");
    printf("成功初始化线程池.\n");

    // 初始化 tcp
    program_stat->socket_fd = init_tcp(program_stat->local_sign, program_stat->thread_stat.thread_resource.queue->len, config_dir, config);
    RET_CHECK_BLACKLIST(-1, program_stat->socket_fd, "init_tcp");
    printf("成功初始化 tcp.\n");

    // 初始化日志功能
    log_init(program_stat->mysql_connect, program_stat->local_sign);

    return 0;
}