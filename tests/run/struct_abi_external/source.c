int printf(char *, ...);

struct Tiny {
    char c;
};

struct Pair {
    int x;
    int y;
};

struct Trio {
    int x;
    int y;
    int z;
};

struct Big {
    int a;
    int b;
    int c;
    int d;
    int e;
};

struct Tiny ext_make_tiny(void);
struct Pair ext_make_pair(void);
struct Trio ext_make_trio(void);
struct Big ext_make_big(void);
int ext_use_tiny(struct Tiny t);
int ext_use_pair(struct Pair p);
int ext_use_trio(struct Trio t);
int ext_use_big(struct Big b);

int main(void) {
    struct Tiny tiny;
    struct Pair pair;
    struct Trio trio;
    struct Big big;
    tiny = ext_make_tiny();
    pair = ext_make_pair();
    trio = ext_make_trio();
    big = ext_make_big();
    printf("%d\n", tiny.c);
    printf("%d\n", pair.x + pair.y);
    printf("%d\n", trio.x + trio.y + trio.z);
    printf("%d\n", big.a + big.b + big.c + big.d + big.e);
    printf("%d\n", ext_use_tiny(tiny));
    printf("%d\n", ext_use_pair(pair));
    printf("%d\n", ext_use_trio(trio));
    printf("%d\n", ext_use_big(big));
    return 0;
}
