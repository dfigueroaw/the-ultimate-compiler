int printf(char *, ...);

int calls = 0;

int next_zero(void) {
  calls = calls + 1;
  return 0;
}

int next_five(void) {
  calls = calls + 1;
  return 5;
}

int must_not_run(void) {
  calls = calls + 100;
  return 1;
}

int main(void) {
  int a;
  int b;
  int c;
  int d;
  int e;
  int f;
  a = 0 && must_not_run();
  b = 7 || must_not_run();
  c = 1 && next_five();
  d = 0 || next_five();
  e = next_zero() && 1;
  f = next_five() || 0;
  printf("%d %d %d %d %d %d %d\n", a, b, c, d, e, f, calls);
  return 0;
}
