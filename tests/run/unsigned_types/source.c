int printf(char *fmt, ...);

int main() {
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long ul;
  unsigned long long ull;
  int signed_cmp;
  int unsigned_cmp;
  int mixed_rank_cmp;
  unsigned int one;
  unsigned int dividend;
  unsigned int divisor;
  unsigned long large_mod;

  uc = 255;
  uc++;
  us = 65535;
  us += 2;
  ui = 0;
  ui--;
  ul = sizeof(unsigned long);
  ull = sizeof(unsigned long long);

  signed_cmp = -1 < 1;
  unsigned_cmp = -1 < ui;
  one = 1;
  mixed_rank_cmp = -1 < one;

  dividend = ui;
  divisor = 2;
  large_mod = 5000000000 % 4294967296;

  printf("%d %d %u %ld %lld\n", uc, us, ui, ul, ull);
  printf("%d %d %d\n", signed_cmp, unsigned_cmp, mixed_rank_cmp);
  printf("%u %u\n", dividend / divisor, dividend % divisor);
  printf("%lu\n", large_mod);
  return 0;
}
