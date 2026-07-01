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

struct Tiny ext_make_tiny(void) {
    struct Tiny t = {65};
    return t;
}

struct Pair ext_make_pair(void) {
    struct Pair p = {3, 4};
    return p;
}

struct Trio ext_make_trio(void) {
    struct Trio t = {5, 6, 7};
    return t;
}

struct Big ext_make_big(void) {
    struct Big b = {1, 2, 3, 4, 5};
    return b;
}

int ext_use_tiny(struct Tiny t) {
    return t.c + 1;
}

int ext_use_pair(struct Pair p) {
    return p.x * 10 + p.y;
}

int ext_use_trio(struct Trio t) {
    return t.x * 100 + t.y * 10 + t.z;
}

int ext_use_big(struct Big b) {
    return b.a + b.b * 10 + b.c * 100 + b.d * 1000 + b.e * 10000;
}
