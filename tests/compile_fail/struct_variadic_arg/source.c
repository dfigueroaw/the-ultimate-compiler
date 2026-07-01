int printf(char *, ...);

struct Point {
    int x;
};

int main(void) {
    struct Point p;
    p.x = 1;
    printf("%d\n", p);
    return 0;
}
