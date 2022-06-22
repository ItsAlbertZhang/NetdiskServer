#ifndef __PROGRESS_BAR_H__
#define __PROGRESS_BAR_H__

#include "head.h"

struct progress_bar_t {
    size_t filesize;
    size_t completedsize;
};

#endif /* __PROGRESS_BAR_H__*/