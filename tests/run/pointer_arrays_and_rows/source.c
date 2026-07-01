int printf(char *, ...);

int main(void) {
    int a[3];
    int b[3];
    int m[2][3];
    int *row;
    int *rows[2];
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    b[0] = 4;
    b[1] = 5;
    b[2] = 6;
    rows[0] = a;
    rows[1] = b;
    m[1][0] = 7;
    m[1][1] = 8;
    m[1][2] = 9;
    row = m[1];
    printf("%d\n", rows[1][2]);
    rows[0][1] = 20;
    printf("%d\n", a[1]);
    printf("%d\n", row[2]);
    return 0;
}
