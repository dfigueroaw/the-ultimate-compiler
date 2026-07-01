int printf(char *, ...);

void *malloc(int);
void free(void *);

struct Pair {
    int left;
    char tag;
    int right;
};

int classify(int value) {
    switch (value) {
    case 0:
    case 1:
        return 10;
    default:
        return 20;
    }
}

int main(void) {
    int *p;
    struct Pair pair;
    p = malloc(sizeof(int));
    *p = 42;
    pair.left = *p;
    pair.tag = -1;
    pair.right = pair.left + pair.tag;
    printf("%d\n", *p);
    printf("%d\n", pair.left);
    printf("%d\n", pair.tag);
    printf("%d\n", pair.right);
    printf("%d\n", classify(0));
    printf("%d\n", classify(2));
    free(p);
    return 0;
}
