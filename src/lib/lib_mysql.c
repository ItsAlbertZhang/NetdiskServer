#include "head.h"
#include "mylibrary.h"

int libmysql_query_count(MYSQL *mysql_connect, const char *query_str) {
    int ret = -1;
    MYSQL_RES *result = NULL;

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

int libmysql_query_1row(MYSQL *mysql_connect, const char *query_str, char *row_p[], int row_cols) {
    int i = 0;
    MYSQL_RES *result = NULL;

    if (!mysql_query(mysql_connect, query_str)) {
        result = mysql_use_result(mysql_connect);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (NULL != row) {
                for (i = 0; i < row_cols && i < mysql_num_fields(result); i++) {
                    strcpy(row_p[i], row[i]);
                }
            }
        }
    }

    mysql_free_result(result);
    return i;
}

int libmysql_query_1col(MYSQL *mysql_connect, const char *query_str, char *col_p[], int col_rows) {
    int i = 0;
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;

    if (!mysql_query(mysql_connect, query_str)) {
        result = mysql_use_result(mysql_connect);
        if (result) {
            while ((row = mysql_fetch_row(result)) != NULL) {
                // 按行循环提取查询结果. 如果查询结果为空则不会进入循环.
                strcpy(col_p[i++], row[0]);
            }
        }
    }

    mysql_free_result(result);
    return i;
}