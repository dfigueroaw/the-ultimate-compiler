int printf(char *, ...);

short gs = 32000;
long gl = 5000000000;

struct Mixed {
    char c;
    short s;
    int i;
    long l;
};

short add_short(short a, short b) {
    short c;
    c = a + b;
    return c;
}

long add_long(long a, long b) {
    return a + b;
}

long scale_long(long value) {
    return value * 100000;
}

int compare_long(long a, long b) {
    return a > b;
}

int main(void) {
    short s;
    short t;
    int promoted;
    long big;
    long from_int;
    struct Mixed m;
    s = 32000;
    t = 1000;
    promoted = s + t;
    printf("%d\n", sizeof(short));
    printf("%d\n", sizeof(long));
    printf("%d\n", sizeof(struct Mixed));
    printf("%d\n", promoted);
    s = promoted;
    printf("%d\n", s);
    printf("%d\n", add_short(100, 23));
    big = 3000000000;
    printf("%ld\n", big);
    printf("%ld\n", add_long(big, 5));
    printf("%ld\n", 5000000000 + 5);
    printf("%ld\n", scale_long(30000));
    from_int = (1 - 2) + 0;
    printf("%ld\n", from_int);
    s = 32767;
    printf("%d\n", ++s);
    printf("%d\n", compare_long(5000000000, 4000000000));
    printf("%d\n", compare_long(1, 4000000000));
    printf("%d\n", gs);
    printf("%ld\n", gl);
    m.c = 1;
    m.s = 2;
    m.i = 3;
    m.l = 4;
    printf("%ld\n", m.c + m.s + m.i + m.l);
    return 0;
}
