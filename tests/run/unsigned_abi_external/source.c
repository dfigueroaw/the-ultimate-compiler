int printf(char *fmt, ...);
unsigned char bump_uc(unsigned char value);
unsigned short bump_us(unsigned short value);
unsigned int half_ui(unsigned int value);
unsigned long long high_ull(unsigned long long value);

int main() {
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long long ull;

  uc = bump_uc(255);
  us = bump_us(65535);
  ui = half_ui(4294967295);
  ull = high_ull(9223372036854775807);

  printf("%u %u %u %llu\n", uc, us, ui, ull);
  return 0;
}
