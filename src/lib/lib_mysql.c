#include "head.h"
#include "mylibrary.h"

int libmysql_dupnum_value(MYSQL *mysql_connect, const char *tbname, const char *fieldname, const char *value) {
    int ret = -1;
    MYSQL_RES *result = NULL;

    char query_str[1024] = {0};
    sprintf(query_str, "SELECT COUNT(*) FROM `%s` WHERE `%s` = '%s';", tbname, fieldname, value);

    if (!mysql_query(mysql_connect, query_str)) {
        result = mysql_use_result(mysql_connect);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            ret = atoi(row[0]);
        }
    }

    mysql_free_result(result);
    return ret;
}
