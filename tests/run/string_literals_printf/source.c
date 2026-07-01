int printf(char *, ...);

char *global_message = "global";

int main(void) {
    char *local;
    local = "local";
    printf("%s %s %d %d\n", global_message, local, sizeof("abc"), local[1]);
    return 0;
}
