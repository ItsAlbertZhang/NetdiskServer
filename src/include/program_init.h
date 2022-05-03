#ifndef __PROGRAM_INIT_H__
#define __PROGRAM_INIT_H__

#define MAX_CONFIG_ROWS 16
#define MAX_CONFIG_LENGTH 256

#include <head.h>

// 读取配置文件前, 获取配置文件目录
int getconfig_init(char *dir, int dirlen);
// 读取配置文件
int getconfig(const char *config_dir, const char *filename, char config[][MAX_CONFIG_LENGTH]);

// 初始化并获取 rsa 密钥
int init_rsa_keys(RSA **private_rsa, RSA **public_rsa, const char *config_dir);

// 初始化 MySQL 数据库连接
int init_mysql(MYSQL **mysql_connect, const char *config_dir, RSA *rsa_private, RSA *rsa_public, char config[][MAX_CONFIG_LENGTH]);

#endif /* __PROGRAM_INIT_H__ */