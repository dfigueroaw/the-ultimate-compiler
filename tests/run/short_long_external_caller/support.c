#include <stdio.h>

short cc_make_short_value(void);
long cc_make_long_value(void);
int cc_accept_short(short value);
long cc_accept_long(long value);

int main(void) {
    short s = cc_make_short_value();
    long l = cc_make_long_value();
    printf("%d\n", s);
    printf("%ld\n", l);
    printf("%d\n", cc_accept_short(s));
    printf("%ld\n", cc_accept_long(l));
    return 0;
}
