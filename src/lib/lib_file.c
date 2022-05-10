#include "head.h"

int file_exist(const char *dir, const char *filename) {
    int ret = 0;
    char fullname[1024];
    // 拼接完整路径
    if (NULL == dir) {
        strcpy(fullname, filename);
    } else {
        sprintf(fullname, "%s%s", dir, filename);
    }
    // 尝试创建文件
    ret = open(fullname, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (ret > 0) {
        // 文件不存在
        close(ret);       // 关闭刚刚创建的文件
        unlink(fullname); // 删除刚刚创建的文件
        ret = 0;
    } else {
        ret = 1;
    }
    return ret;
}

int read_string_from_file(char *str, int maxlen, const char *dir, const char *filename) {
    int ret = 0;
    char fullname[1024];
    // 拼接完整路径
    if (NULL == dir) {
        strcpy(fullname, filename);
    } else {
        sprintf(fullname, "%s%s", dir, filename);
    }
    // 读文件
    int fd = open(fullname, O_RDONLY);
    RET_CHECK_BLACKLIST(-1, fd, "open");
    ret = read(fd, str, maxlen);
    RET_CHECK_BLACKLIST(-1, ret, "read");

    close(fd);

    return ret;
}

int write_file_from_string(const char *str, int len, const char *dir, const char *filename) {
    int ret = 0;
    char fullname[1024];
    // 拼接完整路径
    if (NULL == dir) {
        strcpy(fullname, filename);
    } else {
        sprintf(fullname, "%s%s", dir, filename);
    }
    // 创建或覆盖文件并写入
    int fd = open(fullname, O_RDWR | O_CREAT | O_TRUNC, 0666);
    RET_CHECK_BLACKLIST(-1, fd, "open");
    ftruncate(fd, len);
    ret = write(fd, str, len);
    RET_CHECK_BLACKLIST(-1, ret, "write");

    close(fd);

    return ret;
}