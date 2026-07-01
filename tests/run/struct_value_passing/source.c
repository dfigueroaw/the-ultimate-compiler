int printf(char *, ...);

struct Point {
    int x;
    int y;
};

struct Pair {
    struct Point p;
    char tag;
};

struct Point make_point(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

struct Point copy_point(struct Point p) {
    return p;
}

int sum_point(struct Point p) {
    return p.x + p.y;
}

int sum_pair(struct Pair pair) {
    return pair.p.x + pair.p.y + pair.tag;
}

int sum_point_with_extra(struct Point p, int extra) {
    return p.x + p.y + extra;
}

int main(void) {
    struct Point a;
    struct Point b;
    struct Pair pair;
    a.x = 3;
    a.y = 4;
    b = a;
    printf("%d\n", sum_point(b));
    b = make_point(8, 9);
    printf("%d\n", b.x);
    printf("%d\n", b.y);
    b = copy_point(a);
    printf("%d\n", b.x);
    printf("%d\n", b.y);
    pair.p = b;
    pair.tag = -1;
    printf("%d\n", sum_pair(pair));
    printf("%d\n", sum_point_with_extra(b, 7));
    return 0;
}
