#include "head.h"
#include "main.h"

int main(int argc, const char *argv[]) {
    int ret = 0;
    
    ret = program_init(); // 程序初始化函数
    RET_CHECK_BLACKLIST(-1, ret, "program_init");

    // ret = main_thread_handle(); // 主线程功能函数
    // RET_CHECK_BLACKLIST(-1, ret, "main_thread_handle");

    return 0;
}