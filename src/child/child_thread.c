#include "head.h"

void *child_handle(void *args) {
    printf("%s:%d: child ready.\n", __FILE__, __LINE__);
    return NULL;
}