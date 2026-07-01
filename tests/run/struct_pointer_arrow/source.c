int printf(char *, ...);

struct Rectangulo {
    int ancho;
    int alto;
};

int area(struct Rectangulo *r) {
    return r->ancho * r->alto;
}


int main(void) {
    struct Rectangulo caja;
    caja.ancho = 3;
    caja.alto = 4;
    printf("%d\n", area(&caja));
    return 0;
}
