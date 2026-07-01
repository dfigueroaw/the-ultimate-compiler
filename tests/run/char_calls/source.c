int printf(char *, ...);

int mix(char c, int x, int *p) {
    printf("%d\n", sizeof(c));
    printf("%d\n", sizeof(x));
    printf("%d\n", sizeof(p));
    return c + x + *p;
}

char echo(char c) {
    return c;
}

int main(void) {
    int value;
    value = 5;
    printf("%d\n", mix('A', 7, &value));
    printf("%d\n", echo('B'));
    return 0;
}
