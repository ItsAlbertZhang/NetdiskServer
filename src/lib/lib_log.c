#include "head.h"
#include "mylibrary.h"

char logbuf[4096] = {0};

static MYSQL *mysql_connect = NULL;
static char *local_sign = NULL;

int log_mysql_init(MYSQL *arg_mysql_connect, char *arg_local_sign) {
    mysql_connect = arg_mysql_connect;
    local_sign = arg_local_sign;
    return 0;
}

int logging(int type, const char *str) {
    static char type_str[5][10] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    static char type_str_ff[5][10] = {"\e[90m", "", "\e[43m", "\e[41m", "\e[41m"};
    static char type_str_fb[5][10] = {"\e[0m", "\e[0m ", "\e[0m ", "\e[0m", "\e[0m"};
    int ret = 0, dtype = 1;
#ifdef DEBUG
    dtype = 0;
#endif

    if (type >= dtype) {
        time_t now = time(&now);
        struct tm now_tm;
        gmtime_r(&now, &now_tm);

        printf("\e[100m[%02d:%02d:%02d]\e[0m%s[%s]%s %s\n", now_tm.tm_hour + 8, now_tm.tm_min, now_tm.tm_sec, type_str_ff[type], type_str[type], type_str_fb[type], str);
#ifdef PROD
        if (mysql_connect) {
            char query[1024] = {0};
            sprintf(query, "INSERT INTO log_main(server_sign, type, log) VALUES('%s', '%s', '%s');", local_sign, type_str[type], str);
            ret = mysql_query(mysql_connect, query);
        }
#endif
    }

    return ret;
}