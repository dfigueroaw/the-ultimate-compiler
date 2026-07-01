int printf(char *, ...);

struct Pair {
  char c;
  long x;
};

int main() {
  int folded = (2 + 3) * (10 - 4) / 3 + (9 % 4);
  int comparisons = (8 > 3) + (8 <= 3) + (5 == 5) + (6 != 6);
  int logical = (1 && (2 + 2 == 4)) + (0 || !0);
  int sizes = sizeof(int) + sizeof(struct Pair);
  printf("%d %d %d %d\n", folded, comparisons, logical, sizes);
  return 0;
}
