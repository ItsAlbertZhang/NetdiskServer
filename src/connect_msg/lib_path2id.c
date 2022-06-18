#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

int msg_lib_path2id(const char *path, int *id, MYSQL *mysql_connect) {
    int ret = 0;
    char query_str[1024] = {0};

    if ('/' == *path) {
        // 路径以根目录('/')开始, 则将 id 置 0, 并从下一个字符开始递归
        *id = 0; // 将当前目录置为根目录(0)
        ret = msg_lib_path2id(path + 1, id, mysql_connect);
    } else if (0 == *path) {
        // 路径为空(以 '\0' 开始)
        ret = TYPE_DIR;
    } else if (0 == strncmp(path, "./", strlen("./"))) {
        // 路径以当前目录("./")开始
        ret = msg_lib_path2id(path + 2, id, mysql_connect);
    } else {
        // 路径不以 '/' 和 '\0' 开始
        const char *p = path;
        // 让 p 向右移动, 直至遇到 '/' 或 '\0' 为止
        while ('/' != *p && '\0' != *p) {
            p++;
        }
        char filename[64] = {0};
        strncpy(filename, path, p - path); // 获得当前递归需要处理的文件名
        // 查询该文件或目录是否存在
        sprintf(query_str, "SELECT COUNT(*) FROM `user_file` WHERE `preid` = %d AND `filename` = '%s';", *id, filename);
        if (0 == strcmp("..", filename)) {
            sprintf(query_str, "SELECT `preid` FROM `user_file` WHERE `id` = %d;", *id);
            *id = libmysql_query_11count(mysql_connect, query_str);
            if ('\0' == *p) {
                ret = TYPE_DIR;
            }
            if ('/' == *p) {
                ret = msg_lib_path2id(p + 1, id, mysql_connect);
            }
        } else if (0 == libmysql_query_11count(mysql_connect, query_str)) {
            // 文件名有误
            ret = -1;
        } else {
            // 存在. 获取其 id 与类型.
            char id_str[16] = {0};
            char type_str[4] = {0};
            char *res_p[] = {&id_str[0], &type_str[0]};
            sprintf(query_str, "SELECT `id`, `type` FROM `user_file` WHERE `preid` = %d AND `filename` = '%s';", *id, filename);
            ret = libmysql_query_1row(mysql_connect, query_str, res_p, 2);
            RET_CHECK_BLACKLIST(-1, ret, "libmysql_query_1row");
            *id = atoi(id_str); // 将获取到的 id 填入传入传出参数

            if ('\0' == *p && 'f' == *type_str) {
                // 获取了一个文件的 id, 且无更深一级的递归. 返回 TYPE_FILE.
                ret = TYPE_FILE;
            }
            if ('\0' == *p && 'd' == *type_str) {
                // 获取了一个目录的 id, 且无更深一级的递归. 返回 TYPE_DIR.
                ret = TYPE_DIR;
            }
            if ('/' == *p && 'f' == *type_str) {
                // 获取了一个文件的 id, 且试图向更深一级递归. 发生错误.
                ret = -1;
            }
            if ('/' == *p && 'd' == *type_str ) {
                // 获取了一个目录的 id, 且试图向更深一级递归. 递归即可.
                ret = msg_lib_path2id(p + 1, id, mysql_connect);
            }
        }
    }
    return ret;
}