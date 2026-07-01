int printf(char *, ...);

struct Pair {
    int x;
    int y;
};

struct Trio {
    int x;
    int y;
    int z;
};

int use_pair_after_four(int a, int b, int c, int d, struct Pair p, int tail) {
    return a + b + c + d + p.x + p.y + tail;
}

int use_pair_after_five(int a, int b, int c, int d, int e, struct Pair p,
                        int tail) {
    return a + b + c + d + e + p.x + p.y + tail;
}

int use_trio(struct Trio t) {
    return t.x + t.y + t.z;
}

struct Trio make_trio(int x, int y, int z) {
    struct Trio t;
    t.x = x;
    t.y = y;
    t.z = z;
    return t;
}

int main(void) {
    struct Pair p;
    struct Trio t;
    p.x = 6;
    p.y = 7;
    t = make_trio(10, 20, 30);
    printf("%d\n", use_pair_after_four(1, 2, 3, 4, p, 8));
    printf("%d\n", use_pair_after_five(1, 2, 3, 4, 5, p, 8));
    printf("%d\n", use_trio(t));
    return 0;
}
