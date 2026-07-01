int printf(char *, ...);

struct Box {
    int value;
};

int main(void) {
    int x;
    long y;
    short s;
    int values[4];
    int *p;
    struct Box box;
    x = 10;
    x += 5;
    x -= 3;
    x *= 4;
    x /= 6;
    x %= 5;
    printf("%d\n", x);

    y = 5000000000;
    y += 7;
    y %= 1000;
    printf("%ld\n", y);

    s = 32000;
    s += 1000;
    printf("%d\n", s);

    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[1] += 5;
    p = values;
    p += 2;
    *p += 7;
    p -= 1;
    *p -= 3;
    printf("%d\n", values[1]);
    printf("%d\n", values[2]);

    box.value = 9;
    box.value *= 3;
    printf("%d\n", box.value);

    for (x = 0; x < 5; x += 2) {
        printf("%d\n", x);
    }

    return 0;
}
