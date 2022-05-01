#include "program_init.h"
#include "head.h"

int program_init(void) {
    int ret = 0;

    char config_dir[1024];
    ret = getconfig_init(config_dir, sizeof(config_dir));

    char config[MAX_CONFIG_ROWS][MAX_CONFIG_LENGTH];
    ret = getconfig(config_dir, "mysql.config", config);
    RET_CHECK_BLACKLIST(-1, ret, "getconfig");

    for (int i = 0; i < ret; i++) {
        printf("%s\n", config[i]);
    }

    return 0;
}