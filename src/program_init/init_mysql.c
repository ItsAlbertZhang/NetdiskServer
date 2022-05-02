#include "head.h"
#include "program_init.h"

int init_mysql(MYSQL *connect, const char *config_dir) {
    int ret = 0;

    char config[MAX_CONFIG_ROWS][MAX_CONFIG_LENGTH];
    ret = getconfig(config_dir, "mysql.config", config);
    RET_CHECK_BLACKLIST(-1, ret, "getconfig");

    for (int i = 0; i < ret; i++) {
        printf("%s\n", config[i]);
    }

    int mysql_connected_flag = 0;
    while (!mysql_connected_flag) {
    }

    return 0;
}

int generate_pwd_file() {
    
}