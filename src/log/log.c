#include "head.h"

int log_print(const char *str) {
    printf("%s\n", str);
}

int log_mysql(MYSQL *mysql_connect, const char *local_ip, int type, const char *str) {
    static time_t now;
    static struct tm now_tm;
    time(&now);
    gmtime_r(&now, &now_tm);
    static char type_str[10];
    switch (type) {
    case 0:
        strcpy(type_str, "DEBUG");
        break;
    case 1:
        strcpy(type_str, "INFO");
        break;
    case 2:
        strcpy(type_str, "WARN");
        break;
    case 3:
        strcpy(type_str, "ERROR");
        break;
    case 4:
        strcpy(type_str, "FATAL");
        break;
    default:
        break;
    }
    printf("[%d:%d:%d][%s] %s\n", now_tm.tm_hour + 8, now_tm.tm_min, now_tm.tm_sec, type_str, str);

    static char query[1024];
    sprintf(query, "INSERT INTO log_main(server_ip, type, log) VALUES('%s', %d, '%s');", local_ip, type, str);
    return mysql_query(mysql_connect, query);
}