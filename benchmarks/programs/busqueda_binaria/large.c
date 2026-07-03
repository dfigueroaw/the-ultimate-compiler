int printf(char *, ...);

int buscar(int *arr, int lo, int hi, int objetivo) {
    int mid;
    if (lo > hi) {
        return -1;
    }
    mid = (lo + hi) / 2;
    if (arr[mid] == objetivo) {
        return mid;
    }
    if (arr[mid] < objetivo) {
        return buscar(arr, mid + 1, hi, objetivo);
    }
    return buscar(arr, lo, mid - 1, objetivo);
}

int main(void) {
    int arr[100000];
    int i, encontrados;
    i = 0;
    while (i < 100000) {
        arr[i] = i;
        i = i + 1;
    }
    encontrados = 0;
    i = 0;
    while (i < 100000) {
        if (buscar(arr, 0, 99999, i) != -1) {
            encontrados = encontrados + 1;
        }
        i = i + 100;
    }
    printf("%d\n", encontrados);
    return 0;
}
