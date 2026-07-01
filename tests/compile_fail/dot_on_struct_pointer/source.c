struct Point {
    int x;
};

int main(void) {
    struct Point p;
    struct Point *q;
    q = &p;
    q.x = 1;
    return 0;
}
