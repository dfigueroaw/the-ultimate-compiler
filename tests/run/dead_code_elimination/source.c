int printf(char *, ...);

int skipped(void) {
  printf("bad\n");
  return 100;
}

int after_return(void) {
  return 7;
  return skipped();
}

int main(void) {
  int i;
  int total;
  total = 0;

  if (0) {
    total = total + skipped();
  } else {
    total = total + 1;
  }

  if (1) {
    total = total + 2;
  } else {
    total = total + skipped();
  }

  while (0) {
    total = total + skipped();
  }

  for (; 0;) {
    total = total + skipped();
  }

  i = 0;
  while (i < 4) {
    i = i + 1;
    if (i == 2) {
      continue;
      total = total + skipped();
    }
    if (i == 4) {
      break;
      total = total + skipped();
    }
    total = total + i;
  }

  total = total + after_return();
  printf("%d %d\n", total, i);
  return 0;
}
