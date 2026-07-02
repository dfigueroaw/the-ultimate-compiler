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
    int sum;
    char c;
    char d;
    x = -1;
    c = -1;
    printf("%d\n", x);
    printf("%d\n", negative());
    printf("%d\n", c);
    printf("%d\n", negative_char());
    printf("%d\n", narrow());
    printf("%d\n", -5 / 2);
    c = 120;
    d = 10;
    sum = c + d;
    printf("%d\n", sum);
    c = sum;
    printf("%d\n", c);
    printf("%d\n", sizeof(char));
    printf("%d\n", sizeof(int));
    return 0;
}
