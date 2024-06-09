/*
 *  SJTU CS0501H coursework
 *  external key-value database implemented through B+ tree
 *  supports cache & GC
 */

#include <iostream>
#include <cstring>
#include "b_plus_tree.h"
#include "utils/fast_read.h"

int main() {
    std::ios::sync_with_stdio(false);

    int n;
    char key[65];
    int value;

    BPlusTree bpt(false);
    n = read_int();

    for (int i = 0; i < n; ++i) {
        read_str(key);
        if (strcmp(key, "insert") == 0) {
            read_str(key);
            value = read_int();
            bpt.insert(key, value);
        }
        else if (strcmp(key, "delete") == 0) {
            read_str(key);
            value = read_int();
            bpt.remove(key, value);
        }
        else if (strcmp(key, "find") == 0) {
            read_str(key);
            bpt.print_value(key);
        }
        else
            --i;
    }
}
