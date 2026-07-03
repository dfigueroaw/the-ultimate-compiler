int printf(char *, ...);

int calcular(int a, int b, int c, int d) {
    return a + b * c - d;
}

int main(void) {
    int i, a, b, c, d;
    long total;
    a = 2; b = 3; c = 4; d = 1;
    i = 0;
    total = 0;
    while (i < 20000000) {
        total = total + calcular(a, b, c, d);  /* mismo resultado, pero opaco al optimizador */
        i = i + 1;
    }
    printf("%ld\n", total);
    return 0;
}
