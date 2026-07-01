int printf(char *, ...);

short ext_make_short(void);
long ext_make_long(void);
int ext_use_short(short value);
long ext_use_long(long value);

short cc_make_short(void) {
    short s;
    s = -1234;
    return s;
}

long cc_make_long(void) {
    return 7000000000;
}

int cc_use_short(short value) {
    return value + 2000;
}

long cc_use_long(long value) {
    return value + 5;
}

int main(void) {
    short s;
    long l;
    s = ext_make_short();
    l = ext_make_long();
    printf("%d\n", s);
    printf("%ld\n", l);
    printf("%d\n", ext_use_short(s));
    printf("%ld\n", ext_use_long(l));
    return 0;
}
