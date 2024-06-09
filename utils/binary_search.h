#ifndef UTILS_BINARY_SEARCH_H
#define UTILS_BINARY_SEARCH_H

template<typename T>
int binary_search(T *arr, int size, const T &val) {
    int left = 0;
    int right = size - 1;
    int result = size;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (!(arr[mid] < val)) {
            result = mid;
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }
    return result;
}


#endif
