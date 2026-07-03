int printf(char *, ...);

int main(void) {
    int i;
    long total;
    i = 0;
    total = 0;
    while (i < 20000000) {
        total = total + (2 + 3 * 4 - 1);  /* expresión 100% constante: se pliega a 13 */
        i = i + 1;
    }
    printf("%ld\n", total);
    return 0;
}
