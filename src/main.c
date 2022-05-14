#include "main.h"
#include "head.h"
#include "mylibrary.h"

int main(int argc, const char *argv[]) {
    int ret = 0;

    struct program_stat_t program_stat;
    bzero(&program_stat, sizeof(program_stat));
    ret = program_init(&program_stat); // 程序初始化函数
    RET_CHECK_BLACKLIST(-1, ret, "program_init");
    logging(LOG_INFO, "程序成功启动并初始化完毕.");

    ret = thread_main_handle(&program_stat); // 主线程功能函数
    RET_CHECK_BLACKLIST(-1, ret, "main_thread_handle");

    return 0;
}