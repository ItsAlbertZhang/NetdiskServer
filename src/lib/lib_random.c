#include "head.h"

static char random_gen_char(const char *alphanum, int strlen) { // Random string generator function.
    return alphanum[rand() % strlen];
}

int random_gen_str(char *str, int len, int seed) {
    static const char alphanum[] =
        "0123456789"
        "!@#$^&*"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static const int strlen = sizeof(alphanum) - 1;
    srand(time(NULL) ^ seed);
    for (int i = 0; i < len; i++) {
        str[i] = random_gen_char(alphanum, strlen);
    }
    return 0;
}
