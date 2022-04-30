# 设置编译器
CC = gcc
# 源码所在目录
SRC_DIR = src
# 目标文件所在的目录
OBJS_DIR = debug/obj
# 二进制可执行文件所在的目录
BIN_DIR = debug/bin
# 二进制可执行文件名
BIN = netdisk_server

# 获取工程根目录
ROOT_DIR = $(shell pwd)
# 编译选项
CFLAGS = -I $(ROOT_DIR)/$(SRC_DIR)/include
# 链接选项
LFLAGS = -lpthread

# 获取目标文件
OBJS = ${wildcard $(OBJS_DIR)/*.o}
# 导出变量以进行 Makefile 的递归执行
export CC OBJS_DIR ROOT_DIR CFLAGS

all: COMP LINK
# 递归执行子目录下的 Makefile, 生成目标文件
COMP:
	@make -C $(SRC_DIR) --no-print-directory
# 链接目标文件, 生成二进制可执行文件
LINK:
	$(CC) -o $(BIN_DIR)/$(BIN) $(OBJS)

clean:
	rm $(OBJS) $(BIN_DIR)/$(BIN)