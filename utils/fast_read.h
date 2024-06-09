#ifndef UTILS_FAST_READ_H
#define UTILS_FAST_READ_H

#include <cstdio>

int read_int() {
    char ch = getchar();
    int x = 0, f = 1;
    while (ch < '0' || ch > '9') {
        if (ch == '-')
            f = -1;
        ch = getchar();
    }
    while ('0' <= ch && ch <= '9') {
        x = x * 10 + ch - '0';
        ch = getchar();
    }
    return x * f;
}

void read_str(char *str) {
    char ch = getchar();
    int pos = 0;
    while (ch <= ' ')
        ch = getchar();
    while (ch > ' ') {
        str[pos++] = ch;
        ch = getchar();
    }
    str[pos] = 0;
}

#endif
