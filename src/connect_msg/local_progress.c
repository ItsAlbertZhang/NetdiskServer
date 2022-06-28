#include "connect_msg.h"
#include "head.h"
#include "main.h"
#include "mylibrary.h"
#include "thread_main.h"

enum speedps {
    Bps,
    KBps,
    MBps,
    GBps,
    TBps,
};
char progress_output[] = "====================";
char speedps_str[][5] = {
    "B/s",
    "KB/s",
    "MB/s",
    "GB/s",
    "TB/s",
};

static int print_progress_bar(struct program_stat_t *program_stat) {
    // 访问进度条
    struct queue_t *progress_queue = program_stat->thread_stat.thread_resource.progress_queue;
    int tasknum = (progress_queue->len + progress_queue->rear - progress_queue->front) % progress_queue->len;
    struct progress_t **progress_bar = (struct progress_t **)program_stat->thread_stat.thread_resource.progress_queue->elem_array;
    int ret = tasknum;
    // 准备必要资源
    int percent0o1cnt, percent5cnt, avg_speed_num, avg_speed_level;
    size_t lastsecondsize;
    time_t usedtime;

    static time_t lastupdate; // 一个静态局部变量, 用来判断上一次调用该函数的时间

    // 打印出进度条所需的空间
    // 打印区域为: output 行, 提示信息行, 进度条行. 之后是 log 函数自带的 input 行.

    // 打印出 output 行与提示信息行. 结束后光标停在进度条行.
    if (tasknum) {
        sprintf(logbuf, "按任意键以使下载后台进行. 可输入命令 showpg 重新进入本页面并查看下载进度.");
    } else {
        sprintf(logbuf, "没有正在进行的任务.");
    }
    // 即使没有任务, 也留一行进度条行, 以保留最后一个任务的信息.
    // 因此, 此处采用 do while 结构.
    int i = 0;
    do {
        strcat(logbuf, "\033[K\r\n");
        i++;
    } while (i < tasknum);
    // for (int i = 0; i < tasknum; i++) {
    //     strcat(logbuf, "\033[K\r\n");
    // }
    logging(LOG_OUTPUT, logbuf);
    // 结束后, 光标停在 input 行.

    // 将指针从 input 行移动到进度条行, 即上移 n 行. (n 为任务个数)
    // 当任务个数为 0 时, 此时指针同样在 input 行. 需要将其上移 1 行至进度条行.
    // 由于 \033[0A 等价于 \033[1A , 因此此处反而不用做特殊处理.
    printf("\033[%dA", tasknum);

    // 补充说明一下 n 为 0 的问题.
    // 在没有添加 ret 机制, 而是单纯的返回 tasknum 时, 调用函数必须等待 tasknum 为 0, 方可得知进度队列已结束.
    // 考虑到子线程对进度队列有特殊处理: 在任务结束时, sleep 1 秒后再销毁对应的进度队列, 以便主线程读取.
    // 没有添加 ret 机制时, 任务结束后的 1 秒内读取, tasknum 依旧为 1. 只有当任务结束 1 秒后, tasknum 为 0, 才会跳出监视循环.
    // 而添加了 ret 机制后, 先令 ret = tasknum, 之后比较任务的 completesize 与 filesize. 若两者相同, 则说明该任务已完成, 并处在子线程 sleep 的 1 秒内. 此时直接令 ret--, 若 ret 因此减至 0 则直接跳出监视循环. 因此, 这种方式减少了任务完成后 1 秒的监视自动退出时间.
    // 并且添加了 ret 机制后, 由于提前了 1 秒跳出监视循环, 因此几乎不会出现 tasknum 为 0 的情况. 除非在任务数量为 0 时手动 showpg, 或者出现一种极特殊的情况: 1 秒前传输未完成, 而 1 秒后进度队列已被销毁.(子线程中任务完成 1 秒后才会销毁进度队列, 因此不仅要任务完成的时机恰到好处的和轮询的时间点在同一时刻, 还要 cpu 下发时间片不均匀才行.)

    // 打印进度条. 一共打印 n 行. 打印完毕后, 指针停在 input 行.
    for (int i = 0; i < tasknum; i++) {
        // 由于访问目标是一个队列, 队头元素下标未必是 0, 因此需要获取其真实下标.
        int index = (i + progress_queue->front) % progress_queue->len;
        // 子线程已取出独占资源并获取进度条, 但尚未初始化 (概率极低, 但以防万一)
        if (0 == progress_bar[index]->filesize) {
            break;
        }
        // 校准返回值
        if (progress_bar[index]->completedsize == progress_bar[index]->filesize && progress_bar[index]->lastsize) {
            ret--;
        }
        // 进度条与进度百分比部分
        percent5cnt = progress_bar[index]->completedsize * 20 / progress_bar[index]->filesize;
        percent0o1cnt = progress_bar[index]->completedsize * 1000 / progress_bar[index]->filesize;
        // 当前速度部分
        // lastsecondsize = progress_bar[index]->completedsize - progress_bar[index]->lastsize;
        // progress_bar[index]->lastsize = progress_bar[index]->completedsize;
        // instant_speed_num = lastsecondsize;
        // instant_speed_level = Bps;
        // while (instant_speed_num >> 10) {
        //     instant_speed_num >>= 10;
        //     instant_speed_level++;
        // }
        // 平均速度部分
        usedtime = time(NULL) - progress_bar[index]->starttime;
        if (!usedtime) {
            usedtime = 1;
        }
        avg_speed_num = progress_bar[index]->completedsize / usedtime;
        avg_speed_level = Bps;
        while (avg_speed_num >> 10) {
            avg_speed_num >>= 10;
            avg_speed_level++;
        }
        // 打印输出
        printf("\r[%-20.*s] %3d.%1d%%   平均:%4d %4s   %s\r\n", percent5cnt, progress_output, percent0o1cnt / 10, percent0o1cnt % 10, avg_speed_num, speedps_str[avg_speed_level], progress_bar[index]->file_md5);
    }

    // 将指针从 input 行移动到 output 行
    printf("\033[%dA", tasknum + 2);

    lastupdate = time(NULL);

    return ret;
}

int local_progress(struct program_stat_t *program_stat) {
    int ret = 0;

    // struct termios tm;
    // ret = flushmode_enable(&tm);
    // RET_CHECK_BLACKLIST(-1, ret, "flushmode_enable");

    int tasknum_old = 0, tasknum = 0;
    // while (1) {
    tasknum = print_progress_bar(program_stat);
    // if (0 == tasknum) {
    //     break;
    // }
    // 尝试读取一个字节
    // char c;
    // if (read(STDIN_FILENO, &c, sizeof(char)) > 0) {
    //     break;
    // }
    // 如果任务数量变少, 则需要清理最后 dif 行 + input 行
    if (tasknum < tasknum_old) {
        // 将指针向下移动 output + 提示信息 + n_old 行, 停留在新的 input 行.
        printf("\033[%dB", tasknum + 2);
        // 清理. 清理完毕后指针留在旧的 input 行.
        for (int i = 0; i < tasknum_old - tasknum; i++) {
            printf("\033[K\r\n");
        }
        printf("\033[K");
        // 将指针向上移动 n_old + 提示信息 + output 行, 停留在 output 行.
        printf("\033[%dA", tasknum_old + 2);
        tasknum_old = tasknum;
        // continue;
    }
    tasknum_old = tasknum;
    // sleep(1);
    // }

    sprintf(logbuf, "\r\n");
    // 由于 tasknum 为 0 时, 为了保留最后一个任务的数据多打印了一行, 因此至少要追加一次换行.
    // 因此此处采用至少会循环一次的 do while 结构.
    int i = 0;
    do {
        strcat(logbuf, "\r\n");
        i++;
    } while (i < tasknum);
    // for (int i = 0; i < tasknum; i++) {
    //     strcat(logbuf, "\r\n");
    // }
    logging(LOG_INFO, logbuf);

    // ret = flushmode_disable(tm);
    // RET_CHECK_BLACKLIST(-1, ret, "flushmode_disable");

    return 0;
}