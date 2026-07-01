#include <stdio.h>

short ext_make_short(void) {
    return -2222;
}

long ext_make_long(void) {
    return 9000000000L;
}

int ext_use_short(short value) {
    return value - 10;
}

long ext_use_long(long value) {
    return value - 7;
}
