int printf(char *, ...);

int calls = 0;

int next(void) {
  calls = calls + 1;
  return calls + 10;
}

int main(void) {
  int x;
  int y;
  x = 4;
  y = 0;
  y = y + (next() + 0);
  y = y + (0 + next());
  y = y + (next() - 0);
  y = y + (next() * 1);
  y = y + (1 * next());
  y = y + (next() / 1);
  y = y + (x++ + 0);
  y = y + (0 + x++);
  printf("%d %d %d\n", y, x, calls);
  return 0;
}
