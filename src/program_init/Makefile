# 读取当前目录, 当目录为 include 时不进入其中递归
SUBDIRS = $(shell ls -l | grep ^d | awk '{if($$9 !=  "include") print $$9}')
# 拿到当前目录下的所有 .c 文件
CUR_SOURCE = ${wildcard *.c}
# 拿到目标 .o 文件名
CUR_OBJS = ${patsubst %.c, %.o, $(CUR_SOURCE)}

# 在目标文件路径中执行一次遍历搜索. 遍历后在执行编译时会自动进行增量编译.
vpath %.o $(ROOT_DIR)/$(OBJS_DIR)


all : $(CUR_OBJS) $(SUBDIRS)
# 对每一个需要编译的 .o 文件, 执行编译
%.o : %.c
	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@ $(CFLAGS)
# 进入子目录递归执行 Makefile
$(SUBDIRS) : NULL
	@make -C $@ --no-print-directory	
NULL : 