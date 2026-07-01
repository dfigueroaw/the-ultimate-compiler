int printf(char *, ...);

int calls = 0;

int next() {
  calls = calls + 1;
  return 7;
}

int main() {
  int x = 1;
  int value = next() + (2 * 3);
  int logic = 0 && next();
  int after = x++ + (4 + 5);
  printf("%d %d %d %d %d\n", value, logic, after, x, calls);
  return 0;
}
