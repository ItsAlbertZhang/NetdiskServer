# NetdiskServer

Netdisk server project practice.

本项目使用 C 语言进行编写, 并使用 Makefile 进行编译与链接.

## 编译

使用 `make` 命令以进行编译.

## 环境与依赖

本项目运行在 Linux 环境下, 测试环境为 Ubuntu 20.04.

依赖: `-lpthread` , `-lmysqlclient` , `-lssl` , `-lcrypto`.

测试环境下为: apt 获取 mysqlclient 软件包; 编译安装 openssl 1.1.1 软件包.

## 配置 .config 文件

详见 config/README.md .

## 开发手册

详见 Manual.md .