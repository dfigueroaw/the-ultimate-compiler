int printf(char *, ...);

int main(void) {
    int m[3][3];
    int fila = 0,columna,total = 0;
    m[0][0] = 11;
    m[0][1] = 22;
    m[0][2] = 33;
    m[1][0] = 44;
    m[1][1] = -55;
    m[1][2] = 66;
    m[2][0] = 77;
    m[2][1] = 88;
    m[2][2] = 99;
    while (fila < 3) {
        columna = 0;
        while (columna < 3) {
            if (m[fila][columna] < 0) {
                columna = columna + 1;
                continue;
            }
            total = total + m[fila][columna];
            columna = columna + 1;
        }
        fila = fila + 1;
    }
    printf("%d\n", total);
    return 0;
}
