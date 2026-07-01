int printf(char *, ...);

struct Inner {
    char c;
    int x;
};

struct Outer {
    char tag;
    struct Inner inner;
    struct Inner *ptr;
};

int main(void) {
    struct Inner i;
    struct Outer o;
    char values[3];
    values[0] = 'A';
    values[1] = 'B';
    values[2] = 'C';
    i.c = values[0];
    i.x = 44;
    o.tag = values[1];
    o.inner.c = values[2];
    o.inner.x = 33;
    o.ptr = &i;
    printf("%d\n", o.tag);
    printf("%d\n", o.inner.c);
    printf("%d\n", o.inner.x);
    printf("%d\n", o.ptr->x);
    printf("%d\n", sizeof(struct Outer));
    return 0;
}
