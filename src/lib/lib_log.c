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
    static char type_str[7][10] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL", "INPUT", "OUTPUT"};
    static char type_str_ff[7][10] = {"\e[90m", "", "\e[43m", "\e[41m", "\e[41m", "\e[46m", "\e[42m"};
    static char type_str_fb[7][10] = {"\e[0m ", "\e[0m  ", "\e[0m  ", "\e[0m ", "\e[0m ", "\e[0m ", "\e[0m\r\n"};
    int ret = 0, dtype = 1;
#ifdef DEBUG
    dtype = 0;
#endif

    if (type >= dtype) {
        time_t now = time(NULL);
        struct tm now_tm;
        gmtime_r(&now, &now_tm);

        printf("\r\e[100m[%02d:%02d:%02d]\e[0m%s[%s]%s%s\r\n\e[100m[%02d:%02d:%02d]\e[0m%s[%s]%s", now_tm.tm_hour + 8, now_tm.tm_min, now_tm.tm_sec, type_str_ff[type], type_str[type], type_str_fb[type], str, now_tm.tm_hour + 8, now_tm.tm_min, now_tm.tm_sec, type_str_ff[5], type_str[5], type_str_fb[5]);
        fflush(stdout);
#ifdef PROD
        if (mysql_connect) {
            char query_str[1024] = {0};
            sprintf(query_str, "INSERT INTO log_main(server_sign, type, log) VALUES('%s', '%s', '%s');", local_sign, type_str[type], str);
            ret = mysql_query(mysql_connect, query_str);
        }
#endif
    }

    return ret;
}