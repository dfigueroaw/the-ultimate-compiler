int printf(char *fmt, ...);

int main() {
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long ul;

  uc = 255 + 1;
  us = 65535 + 2;
  ui = 0 - 1;
  ul = sizeof(unsigned long) * 4;

  printf("%u %u %u %lu\n", uc, us, ui, ul);
  return 0;
}
