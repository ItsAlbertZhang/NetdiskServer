#include "head.h"
#include "program_init.h"
#include "log.h"
#include "mycrypt.h"

int program_init(void) {
    int ret = 0;

    // 获取配置文件目录
    char config_dir[1024];
    ret = getconfig_init(config_dir, sizeof(config_dir));
    log_handle("成功获取配置文件目录.");

    // 初始化并获取 rsa 密钥
    RSA *private_rsa = NULL, *public_rsa = NULL;
    ret = init_rsa_keys(&private_rsa, &public_rsa, config_dir);
    RET_CHECK_BLACKLIST(-1, ret, "init_rsa_keys");
    log_handle("成功获取 rsa 密钥.");

    // 初始化 MySQL 数据库连接
    MYSQL *mysql_connect_p = NULL;
    ret = init_mysql(mysql_connect_p, config_dir);

    return 0;
}