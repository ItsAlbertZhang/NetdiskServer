#include "head.h"
#include "program_init.h"

int getconfig_init(char *dir, int dirlen) {
    int ret = 0;
    // 如果使用 getcwd, 则获取到的为工作目录, 在不同目录下启动程序会导致工作目录不一致.
    ret = readlink("/proc/self/exe", dir, dirlen);
    RET_CHECK_BLACKLIST(-1, ret, "readlink");
    // 此时 dir 为 "/home/yx/Netdisk/NetdiskServer/debug/bin/netdisk_server"
    // 对 dir 进行字符串处理, 将其修改为 config 目录
    int cnt = 3;
    while (ret > 0 && cnt > 0) {
        ret--;
        if (dir[ret] == '/') {
            cnt--;
        }
    }
    ret++;
    dir[ret] = 0;
    sprintf(dir, "%sconfig/", dir);
    // 最终 dir 为 "/home/yx/Netdisk/NetdiskServer/config/"
    return ret + 7;
}

int getconfig(const char *config_dir, const char *filename, char config[][MAX_CONFIG_LENGTH]) {
    int ret = 0;
    char fullname[1024] = {0};
    sprintf(fullname, "%s%s", config_dir, filename); // 由 config 目录绝对路径与配置文件名拼接得到配置文件绝对路径

    FILE *fp = fopen(fullname, "rb"); // 打开配置文件
    RET_CHECK_BLACKLIST(NULL, fp, "fopen");
    for (ret = 0; ret < MAX_CONFIG_ROWS; ret++) {
        // 按行读取配置文件
        if (NULL == fgets(config[ret], MAX_CONFIG_LENGTH, fp)) {
            break; // 读到文件尾部, 结束
        } else {
            config[ret][strlen(config[ret]) - 1] = 0; // 替换行尾的 \n 为 \0
        }
    }
    fclose(fp); //关闭配置文件
    return ret;
}