int printf(char *, ...);

int bump(int *value) {
    *value = *value + 1;
    return 0;
}

int main(void) {
    int i;
    int total;
    int updates;
    int value;
    int *p;

    total = 0;
    for (i = 0; i < 5; i = i + 1) {
        if (i == 2) {
            continue;
        }
        total = total + i;
    }
    printf("%d\n", total);

    for (i = 0; ; i = i + 1) {
        if (i == 3) {
            break;
        }
        total = total + 10;
    }
    printf("%d\n", total);

    updates = 0;
    for (; i < 5; bump(&i)) {
        updates = updates + 1;
    }
    printf("%d\n", updates);
    printf("%d\n", i);

    p = &value;
    total = 0;
    for (*p = 0; *p < 3; *p = *p + 1) {
        total = total + *p;
    }
    printf("%d\n", total);

    total = 0;
    for (int j = 0, inner = 1; j < 4; j = j + 1) {
        total = total + j + inner;
    }
    printf("%d\n", total);
    return 0;
}
