int printf(char *, ...);

int main(void) {
    int arr[2000];
    int i, j, tmp, n;
    n = 2000;
    i = 0;
    while (i < n) {
        arr[i] = n - i;
        i = i + 1;
    }
    i = 0;
    while (i < n - 1) {
        j = 0;
        while (j < n - i - 1) {
            if (arr[j] > arr[j + 1]) {
                tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    printf("%d %d\n", arr[0], arr[n - 1]);
    return 0;
}
