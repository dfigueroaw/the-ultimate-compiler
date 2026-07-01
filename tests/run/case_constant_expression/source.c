int printf(char *, ...);

int main(void) {
  int x;
  int total;

  x = 2;
  total = 0;
  switch (x) {
  case 1+1:
    total = 10;
    break;
  default:
    total = 99;
  }
  printf("%d\n", total);

  x = 2;
  total = 0;
  switch (x) {
  case -1+3:
    total = 20;
    break;
  default:
    total = 99;
  }
  printf("%d\n", total);

  x = 4;
  total = 0;
  switch (x) {
  case sizeof(int):
    total = 30;
    break;
  default:
    total = 99;
  }
  printf("%d\n", total);

  x = 1;
  total = 0;
  switch (x) {
  case 2-1:
    total = total + 5;
  case 1*2:
    total = total + 10;
    break;
  default:
    total = 99;
  }
  printf("%d\n", total);

  return 0;
}
