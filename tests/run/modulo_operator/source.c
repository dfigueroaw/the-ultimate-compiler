int printf(char *, ...);

int rem_int(int a, int b) {
    return a % b;
}

long rem_long(long a, long b) {
    return a % b;
}

int main(void) {
    short s;
    s = 32767;
    printf("%d\n", rem_int(17, 5));
    printf("%d\n", rem_int(-17, 5));
    printf("%d\n", rem_int(17, -5));
    printf("%d\n", s % 100);
    printf("%ld\n", rem_long(5000000007, 1000));
    printf("%ld\n", rem_long(-5000000007, 1000));
    return 0;
}
