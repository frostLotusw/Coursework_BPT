#ifndef UTILS_HASH_H
#define UTILS_HASH_H

int hash(const char *str) {
    int h = 0;
    for (int i = 0; str[i]; ++i)
        h = (h * 101 + str[i]) & 16777215;
    return h;
}

#endif
