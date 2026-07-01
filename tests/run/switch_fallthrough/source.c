int printf(char *, ...);

int main(void) {
    int x;
    int total;
    x = 1;
    total = 0;
    switch (x) {
    case 1:
        total = total + 10;
    case 2:
        total = total + 20;
        break;
    default:
        total = total + 100;
    }
    printf("%d\n", total);
    x = -1;
    total = 0;
    switch (x) {
    case -1:
        total = total + 3;
    default:
        total = total + 4;
    }
    printf("%d\n", total);
    x = 5;
    total = 0;
    switch (x) {
    case 1:
        total = total + 10;
    default:
        total = total + 40;
    }
    printf("%d\n", total);
    return 0;
}
