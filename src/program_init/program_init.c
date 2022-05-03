#include "program_init.h"
#include "head.h"
#include "log.h"
#include "mycrypt.h"

int program_init(void) {
    int ret = 0;

    // 获取配置文件目录
    char config_dir[1024];
    ret = getconfig_init(config_dir, sizeof(config_dir));
    log_handle("成功获取配置文件目录.");

    char config[MAX_CONFIG_ROWS][MAX_CONFIG_LENGTH];

    // 初始化并获取 rsa 密钥
    RSA *rsa_private = NULL, *rsa_public = NULL;
    ret = init_rsa_keys(&rsa_private, &rsa_public, config_dir);
    RET_CHECK_BLACKLIST(-1, ret, "init_rsa_keys");
    log_handle("成功获取 rsa 密钥.");

    // 初始化 MySQL 数据库连接
    MYSQL *mysql_connect_p = NULL;
    ret = init_mysql(&mysql_connect_p, config_dir, rsa_private, rsa_public, config);
    RET_CHECK_BLACKLIST(-1, ret, "init_mysql");
    log_handle("成功连接 MySQL 数据库.");

    return 0;
}