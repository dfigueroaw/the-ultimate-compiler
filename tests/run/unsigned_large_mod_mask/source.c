int printf(char *fmt, ...);

unsigned long low32(unsigned long value) {
  return value % 4294967296;
}

int main() {
  printf("%lu\n", low32(5000000000));
  return 0;
}
