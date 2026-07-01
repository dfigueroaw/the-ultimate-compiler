#include <stdio.h>

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

struct Tiny cc_make_tiny(void);
struct Pair cc_make_pair(void);
struct Trio cc_make_trio(void);
struct Big cc_make_big(void);
int cc_use_tiny(struct Tiny t);
int cc_use_pair(struct Pair p);
int cc_use_trio(struct Trio t);
int cc_use_big(struct Big b);

int main(void) {
    struct Tiny tiny = cc_make_tiny();
    struct Pair pair = cc_make_pair();
    struct Trio trio = cc_make_trio();
    struct Big big = cc_make_big();
    printf("%d\n", tiny.c);
    printf("%d\n", pair.x + pair.y);
    printf("%d\n", trio.x + trio.y + trio.z);
    printf("%d\n", big.a + big.b + big.c + big.d + big.e);
    printf("%d\n", cc_use_tiny(tiny));
    printf("%d\n", cc_use_pair(pair));
    printf("%d\n", cc_use_trio(trio));
    printf("%d\n", cc_use_big(big));
    return 0;
}
