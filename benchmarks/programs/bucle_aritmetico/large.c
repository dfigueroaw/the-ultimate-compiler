int printf(char *, ...);

int main(void) {
    int i;
    long total;
    i = 0;
    total = 0;
    while (i < 50000000) {
        total = total + i * 2 - 1;
        i = i + 1;
    }
    printf("%ld\n", total);
    return 0;
}
