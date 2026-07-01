int printf(char *, ...);

struct Inner {
    char c;
    int x;
};

struct Holder {
    char tag;
    int values[3];
    struct Inner inner;
    struct Inner *ptr;
};

int value(void) {
    return 1;
}

int main(void) {
    int a[2];
    int m[2][3];
    int *p;
    struct Inner i;
    struct Holder h;
    p = a;
    h.ptr = &i;
    printf("%d\n", sizeof(char));
    printf("%d\n", sizeof(int));
    printf("%d\n", sizeof(void *));
    printf("%d\n", sizeof(struct Inner));
    printf("%d\n", sizeof(struct Holder));
    printf("%d\n", sizeof(a));
    printf("%d\n", sizeof(m));
    printf("%d\n", sizeof(m[0]));
    printf("%d\n", sizeof(m[0][0]));
    printf("%d\n", sizeof(p));
    printf("%d\n", sizeof(*p));
    printf("%d\n", sizeof(&a));
    printf("%d\n", sizeof(h.values));
    printf("%d\n", sizeof(h.inner));
    printf("%d\n", sizeof(h.ptr->x));
    printf("%d\n", sizeof(p + 1));
    printf("%d\n", sizeof(value()));
    return 0;
}
