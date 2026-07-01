int printf(char *, ...);

struct Resultado {
    int total;
    int mayor;
};

int sumaLista(int *valores, int n) {
    int i = 0,total = 0;
    while (i < n) {
        total = total + valores[i];
        i = i + 1;
    }
    return total;
}

int mayorLista(int *valores, int n) {
    int i = 1,mayor = valores[0];
    while (i < n) {
        if (valores[i] > mayor) {
            mayor = valores[i];
        }
        i = i + 1;
    }
    return mayor;
}

int main(void) {
    int datos[4];
    struct Resultado r;
    datos[0] = 4;
    datos[1] = 8;
    datos[2] = 3;
    datos[3] = 9;
    r.total = sumaLista(datos, 4);
    r.mayor = mayorLista(datos, 4);
    printf("%d\n", r.total);
    printf("%d\n", r.mayor);
    return 0;
}
