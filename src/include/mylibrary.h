#ifndef __MYLIBRARY_H__
#define __MYLIBRARY_H__

// file.c

// 检查文件是否存在, 存在返回 1, 不存在返回 0. dir 可以为 NULL.
int file_exist(const char *dir, const char *filename);

// 从文件中读取字符串并写入 str, 长度至多为 maxlen, 返回值为成功读取长度. dir 可以为 NULL.
int read_string_from_file(char *str, int maxlen, const char *dir, const char *filename);

// 将字符串写入文件, 长度为 len, 返回值为实际写入长度. dir 可以为 NULL.
int write_file_from_string(const char *str, int len, const char *dir, const char *filename);

// queue.c

// 队列结构体
struct queue_t {
    int *queue; // 队列数组, 数据类型为 int
    int len;    // 队列长度
    int front;  // front 为队头元素下标
    int rear;   // rear 为队尾元素下标 + 1
    char tag;   // tag 为 1 代表上一次为入队操作, 为 0 代表为出队操作.
};

// 初始化队列
int queue_init(struct queue_t **pQ, int len);

// 销毁队列
int queue_destroy(struct queue_t **pQ);

// 入队
int queue_in(struct queue_t *Q, int elem);

// 出队
int queue_out(struct queue_t *Q, int *elem);

// rsa.c

#define PRIKEY 0
#define PUBKEY 1

int rsa_encrypt(const unsigned char *plaintext, unsigned char *ciphertext, RSA *rsa, int rsa_type);
int rsa_decrypt(unsigned char *plaintext, const unsigned char *ciphertext, RSA *rsa, int rsa_type);

// log.c

// 打印日志
int log_print(const char *string);

// 打印日志并保存至数据库
int log_mysql(MYSQL *mysql_connect, const char *local_ip, int type, const char *str);

// epoll.c

// 添加 epoll 监听
int epoll_add(int epfd, int fd);

// 移除 epoll 监听
int epoll_del(int epfd, int fd);

#endif /* __MYLIBRARY_H__ */