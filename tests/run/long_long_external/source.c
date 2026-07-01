int printf(char *fmt, ...);

long long external_mix(long long a, long long b, int c);

int main() {
    long long result;
    result = external_mix(4000000000, 11, 5);
    printf("%lld\n", result);
    return 0;
}
