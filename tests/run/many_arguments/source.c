int printf(char *, ...);

int one(void) {
    return 1;
}

int two(void) {
    return 2;
}

int three(void) {
    return 3;
}

int sum8(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

int mixed(int a, int b, int c, int d, int e, int f, char g, int *p, int h) {
    return a + b + c + d + e + f + g + *p + h;
}

int main(void) {
    int value;
    value = 10;
    printf("%d\n", sum8(1, 2, 3, 4, 5, 6, 7, 8));
    printf("%d\n", sum8(one(), two(), three(), 4, 5, 6, one() + two(), 8));
    printf("%d\n", mixed(1, 2, 3, 4, 5, 6, -1, &value, 9));
    return 0;
}
