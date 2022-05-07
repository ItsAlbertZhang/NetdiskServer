#ifndef __LOG_H__
#define __LOG_H__

// 打印日志
int log_print(const char *string);

// 打印日志并保存至数据库
int log_mysql(MYSQL *mysql_connect, const char *local_ip, int type, const char *str);

#endif /* __LOG_H__ */