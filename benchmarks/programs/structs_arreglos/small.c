int printf(char *, ...);

struct Punto {
    int x;
    int y;
};

int main(void) {
    struct Punto puntos[10];
    int i, idx;
    long suma;
    i = 0;
    while (i < 10) {
        puntos[i].x = i;
        puntos[i].y = i * 2;
        i = i + 1;
    }
    suma = 0;
    i = 0;
    while (i < 500000) {
        idx = i % 10;
        suma = suma + puntos[idx].x + puntos[idx].y;
        i = i + 1;
    }
    printf("%ld\n", suma);
    return 0;
}
