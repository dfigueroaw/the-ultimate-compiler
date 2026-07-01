int printf(char *, ...);

struct Buffer {
    char name[4];
    int values[2];
};

struct Point {
    char tag;
    int x;
};

int main(void) {
    struct Buffer b;
    struct Point points[2];
    int nums[4];
    int *p;
    int *q;
    b.name[0] = 'A';
    b.name[1] = 'B';
    b.values[0] = 7;
    b.values[1] = 9;
    points[0].x = 11;
    points[1].x = 22;
    nums[0] = 10;
    nums[1] = 20;
    nums[2] = 30;
    p = nums;
    q = p + 2;
    printf("%d\n", b.name[1]);
    printf("%d\n", b.values[1]);
    printf("%d\n", points[1].x);
    printf("%d\n", *q);
    printf("%d\n", q - p);
    printf("%d\n", *(1 + p));
    return 0;
}
