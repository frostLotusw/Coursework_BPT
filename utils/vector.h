#ifndef UTILS_VECTOR_H
#define UTILS_VECTOR_H

#include <utility>

template<class T>
class Vector {

protected:

    int pos;
    int space;
    T *data;

    void expand(int req_space = 0) {
        do
            space *= 2;
        while (space < req_space);

        T *new_data = new T[space];
        for (int i = 0; i < pos; ++i)
            new_data[i] = data[i];

        delete[] data;
        data = new_data;
    }

public:

    Vector() : pos(0), space(1), data(new T[1]) {}

    virtual ~Vector() {
        delete[] data;
    }

    template<typename U>
    void push_back(U &&val) {
        if (pos >= space)
            expand();
        data[pos++] = std::forward<U>(val);
    }

    void resize(int new_size) {
        if (new_size >= space)
            expand(new_size);
        pos = new_size;
    }

    T &operator[](int index) const {
        return data[index];
    }

    int size() const {
        return pos;
    }
};

#endif
