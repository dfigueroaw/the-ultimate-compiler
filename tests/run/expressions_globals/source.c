int printf(char *, ...);

int global_base = 10;
int *global_ptr = 0;

void mostrar(int valor) {
    printf("%d\n", valor);
    return;
}

int main(void) {
    int x = 1+2+3+4+5+6+7+8+9+10;
    int *p = 0;
    if (!p) {
        mostrar(sizeof(x + global_base));
    }
    if (global_ptr == 0) {
        mostrar(x);
    }
    return 0;
}
