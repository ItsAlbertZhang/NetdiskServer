#ifndef __MAIN_H__
#define __MAIN_H__

#include "program_stat.h"

int program_init(struct program_stat_t *program_stat);

int thread_main_handle(struct program_stat_t *program_stat);

#endif /* __MAIN_H__ */