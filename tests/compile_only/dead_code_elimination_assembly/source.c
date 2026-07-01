int printf(char *, ...);

int main(void) {
  if (0) {
    printf("unreachable-branch\n");
  }
  while (0) {
    printf("unreachable-loop\n");
  }
  return 0;
  printf("unreachable-return\n");
}
