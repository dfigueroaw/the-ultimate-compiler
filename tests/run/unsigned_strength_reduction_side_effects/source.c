int printf(char *fmt, ...);

int calls;

unsigned int value() {
  calls++;
  return 17;
}

int main() {
  unsigned int q;
  unsigned int r;
  unsigned int product;

  q = value() / 8;
  r = value() % 8;
  product = value() * 0;
  q /= 2;
  r %= 2;

  printf("%d %u %u %u\n", calls, q, r, product);
  return 0;
}
