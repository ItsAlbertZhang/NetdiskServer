#ifndef __HEAD_H__
#define __HEAD_H__

#define _GNU_SOURCE // crypt
#include <arpa/inet.h>
#include <crypt.h>
// #include <dirent.h>
// #include <errno.h>
#include <fcntl.h>
// #include <grp.h>
// #include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
// #include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
// #include <sys/ipc.h>
// #include <sys/mman.h>
// #include <sys/msg.h>
// #include <sys/select.h>
// #include <sys/sem.h>
#include <sys/sendfile.h>
// #include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
// #include <sys/time.h>
#include <sys/types.h>
// #include <sys/wait.h>
#include <mysql/mysql.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <termio.h>
#include <time.h>
#include <unistd.h>

#define RET_CHECK_BLACKLIST(error_ret, ret, funcname)                   \
    {                                                                   \
        if (error_ret == ret) {                                         \
            printf("%s:%d: %s error.\n", __FILE__, __LINE__, funcname); \
            return -1;                                                  \
        }                                                               \
    }

#define THREAD_RET_CHECK(ret, funcname)                                                   \
    {                                                                                     \
        if (ret != 0) {                                                                   \
            printf("%s:%d: %s error: %s\n", __FILE__, __LINE__, funcname, strerror(ret)); \
            return -1;                                                                    \
        }                                                                                 \
    }

#endif
