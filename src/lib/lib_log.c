#include "head.h"
#include "mylibrary.h"

char logbuf[4096] = {0};

static MYSQL *mysql_connect = NULL;
static char *local_sign = NULL;

int log_init(MYSQL *arg_mysql_connect, char *arg_local_sign) {
    mysql_connect = arg_mysql_connect;
    local_sign = arg_local_sign;
    return 0;
}

int logging(int type, const char *str) {
    static char type_str[5][10] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    static char type_str_format[5][2] = {"", " ", " ", "", ""};
    int ret = 0, dtype = 1;
#ifdef DEBUG
    dtype = 0;
#endif

    if (type >= dtype) {
        time_t now = time(&now);
        struct tm now_tm;
        gmtime_r(&now, &now_tm);

        printf("[%02d:%02d:%02d][%s]%s %s\n", now_tm.tm_hour + 8, now_tm.tm_min, now_tm.tm_sec, type_str[type], type_str_format[type], str);
#ifdef PROD
        char query[1024] = {0};
        sprintf(query, "INSERT INTO log_main(server_sign, type, log) VALUES('%s', '%s', '%s');", local_sign, type_str[type], str);
        ret = mysql_query(mysql_connect, query);
#endif
    }

    return ret;
}