int printf(char *fmt, ...);

signed explicit_signed(signed value) {
  signed result;
  result = value / 2;
  return result;
}

int main() {
  signed char sc;
  signed short ss;
  signed int si;
  signed long sl;
  signed long long sll;

  sc = 255;
  ss = 65535;
  si = explicit_signed(-7);
  sl = sizeof(signed long);
  sll = sizeof(signed long long);

  printf("%d %d %d %ld %lld\n", sc, ss, si, sl, sll);
  return 0;
}
