int printf(char *fmt, ...);

struct LongLongPair {
    long long first;
    long second;
};

long long add_long_long(long long a, long b) {
    return a + b;
}

long long take_pair(struct LongLongPair pair) {
    return pair.first - pair.second;
}

int main() {
    long long base;
    long long int alias;
    long small;
    short tiny;
    long long values[2];
    struct LongLongPair pair;

    base = 2147483648;
    alias = 9;
    small = 7;
    tiny = 3;
    values[0] = base + alias;
    values[1] = add_long_long(values[0], small);
    pair.first = values[1] * tiny;
    pair.second = small;

    printf("%lld %lld %d %d %d\n",
           values[1],
           take_pair(pair),
           sizeof(long long),
           sizeof(long long int *),
           sizeof(values));
    return 0;
}
