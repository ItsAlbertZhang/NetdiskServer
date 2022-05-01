#ifndef __PROGRAM_INIT_H__
#define __PROGRAM_INIT_H__

#define MAX_CONFIG_ROWS 16
#define MAX_CONFIG_LENGTH 256

// 读取配置文件前, 获取配置文件目录
int getconfig_init(char *dir, int dirlen);
// 读取配置文件
int getconfig(char *config_dir, char *filename, char config[][MAX_CONFIG_LENGTH]);

#endif /* __PROGRAM_INIT_H__ */