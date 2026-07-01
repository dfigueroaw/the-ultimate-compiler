int printf(char *, ...);

int main(void) {
    int x;
    int y;
    int values[3];
    int *p;
    char c;

    x = 1;
    printf("%d\n", ++x);
    printf("%d\n", x++);
    printf("%d\n", x);
    printf("%d\n", --x);
    printf("%d\n", x--);
    printf("%d\n", x);

    y = x++ + 10;
    printf("%d\n", y);
    printf("%d\n", x);
    y = ++x + 10;
    printf("%d\n", y);
    printf("%d\n", x);

    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    p = values;
    printf("%d\n", *p++);
    printf("%d\n", *p);
    printf("%d\n", *++p);
    printf("%d\n", p - values);
    p--;
    printf("%d\n", *p);

    c = 1;
    printf("%d\n", c++);
    printf("%d\n", c);
    return 0;
}
