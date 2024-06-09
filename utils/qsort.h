#ifndef UTILS_QSORT_H
#define UTILS_QSORT_H

template<typename T>
void qsort(T *start, T *end) {

    if (end - start <= 1)
        return;
    T piv = *(start + (end - start) / 2);
    T tmp;
    T *l = start - 1, *r = end, *m = start;

    while (m < r) {
        if (*m < piv) {
            ++l;
            tmp = *l;
            *l = *m;
            *m = tmp;
            ++m;
        }
        else if (piv < *m) {
            --r;
            tmp = *r;
            *r = *m;
            *m = tmp;
        }
        else ++m;
    }

    qsort(start, l + 1);
    qsort(r, end);
}

template<typename T>
void qsort(T *start, T *end, bool (*comp)(const T &a, const T &b)) {

    if (end - start <= 1)
        return;
    T piv = *(start + (end - start) / 2);
    T tmp;
    T *l = start - 1, *r = end, *m = start;

    while (m < r) {
        if (comp(*m, piv)) {
            ++l;
            tmp = *l;
            *l = *m;
            *m = tmp;
            ++m;
        }
        else if (comp(piv, *m)) {
            --r;
            tmp = *r;
            *r = *m;
            *m = tmp;
        }
        else ++m;
    }

    qsort(start, l + 1, comp);
    qsort(r, end, comp);
}

#endif
