short cc_make_short_value(void) {
    short s;
    s = -1234;
    return s;
}

long cc_make_long_value(void) {
    return 7000000000;
}

int cc_accept_short(short value) {
    return value + 2000;
}

long cc_accept_long(long value) {
    return value + 5;
}
