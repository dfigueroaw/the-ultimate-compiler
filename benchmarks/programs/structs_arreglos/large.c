int printf(char *, ...);

struct Punto {
    int x;
    int y;
};

int main(void) {
    struct Punto puntos[1000];
    int i;
    long suma;
    i = 0;
    while (i < 1000) {
        puntos[i].x = i;
        puntos[i].y = i * 2;
        i = i + 1;
    }
    suma = 0;
    i = 0;
    while (i < 1000) {
        suma = suma + puntos[i].x + puntos[i].y;
        i = i + 1;
    }
    printf("%ld\n", suma);
    return 0;
}
