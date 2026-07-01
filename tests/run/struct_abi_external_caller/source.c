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

struct Tiny cc_make_tiny(void) {
    struct Tiny t;
    t.c = 70;
    return t;
}

struct Pair cc_make_pair(void) {
    struct Pair p;
    p.x = 8;
    p.y = 9;
    return p;
}

struct Trio cc_make_trio(void) {
    struct Trio t;
    t.x = 10;
    t.y = 11;
    t.z = 12;
    return t;
}

struct Big cc_make_big(void) {
    struct Big b;
    b.a = 2;
    b.b = 3;
    b.c = 4;
    b.d = 5;
    b.e = 6;
    return b;
}

int cc_use_tiny(struct Tiny t) {
    return t.c + 2;
}

int cc_use_pair(struct Pair p) {
    return p.x * 10 + p.y;
}

int cc_use_trio(struct Trio t) {
    return t.x * 100 + t.y * 10 + t.z;
}

int cc_use_big(struct Big b) {
    return b.a + b.b * 10 + b.c * 100 + b.d * 1000 + b.e * 10000;
}
