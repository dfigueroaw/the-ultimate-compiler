int printf(char *, ...);

struct TwoLongs {
    long a;
    long b;
};

struct BigLongs {
    long a;
    long b;
    long c;
};

long many(long a, long b, long c, long d, long e, long f, long g) {
    return a + b + c + d + e + f + g;
}

struct TwoLongs make_two(long a, long b) {
    struct TwoLongs t;
    t.a = a;
    t.b = b;
    return t;
}

long use_two(struct TwoLongs t) {
    return t.a + t.b;
}

struct BigLongs make_big(long a, long b, long c) {
    struct BigLongs t;
    t.a = a;
    t.b = b;
    t.c = c;
    return t;
}

long use_big(struct BigLongs t) {
    return t.a + t.b + t.c;
}

int main(void) {
    struct TwoLongs two;
    struct BigLongs big;
    two = make_two(10000000000, 20);
    big = make_big(1, 20000000000, 3);
    printf("%ld\n", many(1, 2, 3, 4, 5, 6, 7));
    printf("%ld\n", two.a);
    printf("%ld\n", two.b);
    printf("%ld\n", use_two(two));
    printf("%ld\n", use_big(big));
    return 0;
}
