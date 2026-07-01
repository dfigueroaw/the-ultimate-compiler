int printf(char *, ...);

int one(void) {
    return 1;
}

int two(void) {
    return 2;
}

int add(int a, int b) {
    return a + b;
}

int idx(void) {
    return 1;
}

int main(void) {
    int keep;
    int a[2][3];
    keep = 9;
    if (1) {
        int left[100];
        left[99] = 1;
    } else {
        int right[100];
        right[99] = 2;
    }
    a[idx()][2] = 55;
    printf("%d\n", keep);
    printf("%d\n", add(one(), two()));
    printf("%d\n", one() + two());
    printf("%d\n", a[1][2]);
    return 0;
}
