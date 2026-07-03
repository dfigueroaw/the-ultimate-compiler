int printf(char *, ...);

int trabajo_pesado(int n) {
    int i;
    int acc;
    i = 0;
    acc = 0;
    while (i < 1000) {
        acc = acc + i * i;
        i = i + 1;
    }
    return acc;
}

int main(void) {
    int i, resultado, condicion;
    resultado = 0;
    condicion = 0;
    i = 0;
    while (i < 200000000) {
        if (condicion) {
            resultado = resultado + trabajo_pesado(i);
        }
        resultado = resultado + 1;
        i = i + 1;
    }
    printf("%d\n", resultado);
    return 0;
}
