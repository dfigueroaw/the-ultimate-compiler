int printf(char *, ...);

int main(void) {
    char a;
    char b;
    int sum;
    a = 120;
    b = 10;
    sum = a + b;
    printf("%d\n", sum);
    a = sum;
    printf("%d\n", a);
    printf("%d\n", sizeof(char));
    printf("%d\n", sizeof(int));
    return 0;
}
