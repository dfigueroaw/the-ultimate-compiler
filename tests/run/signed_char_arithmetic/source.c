int printf(char *, ...);

int negative(void) {
    return -1;
}

char narrow(void) {
    return 300;
}

char negative_char(void) {
    char c;
    c = -1;
    return c;
}

int main(void) {
    int x;
    char c;
    x = -1;
    c = -1;
    printf("%d\n", x);
    printf("%d\n", negative());
    printf("%d\n", c);
    printf("%d\n", negative_char());
    printf("%d\n", narrow());
    printf("%d\n", -5 / 2);
    return 0;
}
