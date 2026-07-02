int printf(char *fmt, ...);

int main() {
  signed int si;
  unsigned int ui;
  signed long sl;
  unsigned long ul;
  signed char sc;
  unsigned char uc;
  int signed_div;
  unsigned int unsigned_div;
  int signed_mod;
  unsigned int unsigned_mod;

  si = -5;
  ui = 4294967295;
  sl = -1;
  ul = 4294967295;
  sc = 255;
  uc = 255;

  signed_div = si / 2;
  unsigned_div = ui / 2;
  signed_mod = si % 2;
  unsigned_mod = ui % 2;

  printf("%d %u %d %u\n", signed_div, unsigned_div, signed_mod,
         unsigned_mod);
  printf("%d %d %d %d\n", si < ui, si > ui, sl > ui, ul > sl);
  printf("%d %d %d %d\n", sc, uc, sc < uc, uc > sc);
  return 0;
}
