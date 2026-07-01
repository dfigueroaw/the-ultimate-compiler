int printf(char *, ...);
int scanf(char *, ...);

int main(void) {
    int value;
    value = 0;
    scanf("%d", &value);
    printf("%d\n", value + 1);
    return 0;
}
